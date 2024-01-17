//===- ConvergenceRegionAnalysis.h -----------------------------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The analysis determines the convergence region for each basic block of
// the module, and provides a tree-like structure describing the region
// hierarchy.
//
//===----------------------------------------------------------------------===//

#include "ConvergenceRegionAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IntrinsicInst.h"
#include <optional>
#include <queue>

namespace llvm {
namespace SPIRV {

namespace {

template <typename BasicBlockType, typename IntrinsicInstType>
std::optional<IntrinsicInstType *>
getConvergenceTokenInternal(BasicBlockType *BB) {
  static_assert(std::is_const_v<IntrinsicInstType> ==
                    std::is_const_v<BasicBlockType>,
                "Constness must match between input and output.");
  static_assert(std::is_same_v<BasicBlock, std::remove_const_t<BasicBlockType>>,
                "Input must be a basic block.");
  static_assert(
      std::is_same_v<IntrinsicInst, std::remove_const_t<IntrinsicInstType>>,
      "Output type must be an intrinsic instruction.");

  for (auto &I : *BB) {
    if (auto *II = dyn_cast<IntrinsicInst>(&I)) {
      if (II->getIntrinsicID() != Intrinsic::experimental_convergence_entry &&
          II->getIntrinsicID() != Intrinsic::experimental_convergence_loop &&
          II->getIntrinsicID() != Intrinsic::experimental_convergence_anchor) {
        continue;
      }

      if (II->getIntrinsicID() == Intrinsic::experimental_convergence_entry ||
          II->getIntrinsicID() == Intrinsic::experimental_convergence_loop) {
        return II;
      }

      auto Bundle = II->getOperandBundle(LLVMContext::OB_convergencectrl);
      assert(Bundle->Inputs.size() == 1 &&
             Bundle->Inputs[0]->getType()->isTokenTy());
      auto TII = dyn_cast<IntrinsicInst>(Bundle->Inputs[0].get());
      ;
      assert(TII != nullptr);
      return TII;
    }

    if (auto *CI = dyn_cast<CallInst>(&I)) {
      auto OB = CI->getOperandBundle(LLVMContext::OB_convergencectrl);
      if (!OB.has_value())
        continue;
      return dyn_cast<IntrinsicInst>(OB.value().Inputs[0]);
    }
  }

  return std::nullopt;
}

} // anonymous namespace

std::optional<IntrinsicInst *> getConvergenceToken(BasicBlock *BB) {
  return getConvergenceTokenInternal<BasicBlock, IntrinsicInst>(BB);
}

std::optional<const IntrinsicInst *> getConvergenceToken(const BasicBlock *BB) {
  return getConvergenceTokenInternal<const BasicBlock, const IntrinsicInst>(BB);
}

ConvergenceRegion::ConvergenceRegion(DominatorTree &DT, LoopInfo &LI,
                                     Function &F)
    : DT(DT), LI(LI), Parent(nullptr) {
  Entry = &F.getEntryBlock();
  ConvergenceToken = getConvergenceToken(Entry);
  for (auto &B : F) {
    Blocks.insert(&B);
    if (isa<ReturnInst>(B.getTerminator()))
      Exits.insert(&B);
  }
}

ConvergenceRegion::ConvergenceRegion(
    DominatorTree &DT, LoopInfo &LI,
    std::optional<IntrinsicInst *> ConvergenceToken, BasicBlock *Entry,
    SmallPtrSet<BasicBlock *, 8> &&Blocks, SmallPtrSet<BasicBlock *, 2> &&Exits)
    : DT(DT), LI(LI), ConvergenceToken(ConvergenceToken), Entry(Entry),
      Exits(std::move(Exits)), Blocks(std::move(Blocks)) {
  for (auto *BB : this->Exits)
    assert(this->Blocks.count(BB) != 0);
  assert(this->Blocks.count(this->Entry) != 0);
}

void ConvergenceRegion::releaseMemory() {
  // Parent memory is owned by the parent.
  Parent = nullptr;
  for (auto *Child : Children) {
    Child->releaseMemory();
    delete Child;
  }
  Children.resize(0);
}

void ConvergenceRegion::dump(const unsigned IndentSize) const {
  const std::string Indent(IndentSize, '\t');
  dbgs() << Indent << this << ": {\n";
  dbgs() << Indent << "	Parent: " << Parent << "\n";

  if (ConvergenceToken.value_or(nullptr)) {
    dbgs() << Indent
           << "	ConvergenceToken: " << ConvergenceToken.value()->getName()
           << "\n";
  }

  if (Entry->getName() != "")
    dbgs() << Indent << "	Entry: " << Entry->getName() << "\n";
  else
    dbgs() << Indent << "	Entry: " << Entry << "\n";

  dbgs() << Indent << "	Exits: { ";
  for (const auto &Exit : Exits) {
    if (Exit->getName() != "")
      dbgs() << Exit->getName() << ", ";
    else
      dbgs() << Exit << ", ";
  }
  dbgs() << "	}\n";

  dbgs() << Indent << "	Blocks: { ";
  for (const auto &Block : Blocks) {
    if (Block->getName() != "")
      dbgs() << Block->getName() << ", ";
    else
      dbgs() << Block << ", ";
  }
  dbgs() << "	}\n";

  dbgs() << Indent << "	Children: {\n";
  for (const auto Child : Children)
    Child->dump(IndentSize + 2);
  dbgs() << Indent << "	}\n";

  dbgs() << Indent << "}\n";
}

class ConvergenceRegionAnalyzer {

public:
  ConvergenceRegionAnalyzer(Function &F, DominatorTree &DT, LoopInfo &LI)
      : DT(DT), LI(LI), F(F) {}

private:
  bool isBackEdge(const BasicBlock *From, const BasicBlock *To) const {
    assert(From != To && "From == To. This is awkward.");

    // We only handle loop in the simplified form. This means:
    // - a single back-edge, a single latch.
    // - meaning the back-edge target can only be the loop header.
    // - meaning the From can only be the loop latch.
    if (!LI.isLoopHeader(To))
      return false;

    auto *L = LI.getLoopFor(To);
    if (L->contains(From) && L->isLoopLatch(From))
      return true;

    return false;
  }

  std::unordered_set<BasicBlock *>
  findPathsToMatch(BasicBlock *From,
                   std::function<bool(const BasicBlock *)> isMatch) const {
    std::unordered_set<BasicBlock *> Output;

    if (isMatch(From))
      Output.insert(From);

    auto *Terminator = From->getTerminator();
    for (unsigned i = 0; i < Terminator->getNumSuccessors(); ++i) {
      auto *To = Terminator->getSuccessor(i);
      if (isBackEdge(From, To))
        continue;

      auto ChildSet = findPathsToMatch(To, isMatch);
      if (ChildSet.size() == 0)
        continue;

      Output.insert(ChildSet.begin(), ChildSet.end());
      Output.insert(From);
    }

    return Output;
  }

  SmallPtrSet<BasicBlock *, 2>
  findExitNodes(const SmallPtrSetImpl<BasicBlock *> &RegionBlocks) {
    SmallPtrSet<BasicBlock *, 2> Exits;

    for (auto *B : RegionBlocks) {
      auto *Terminator = B->getTerminator();
      for (unsigned i = 0; i < Terminator->getNumSuccessors(); ++i) {
        auto *Child = Terminator->getSuccessor(i);
        if (RegionBlocks.count(Child) == 0)
          Exits.insert(B);
      }
    }

    return Exits;
  }

public:
  ConvergenceRegionInfo analyze() {
    ConvergenceRegion *TopLevelRegion = new ConvergenceRegion(DT, LI, F);

    std::unordered_map<Loop *, ConvergenceRegion *> LoopToRegion;
    std::queue<Loop *> ToProcess;
    for (auto *L : LI)
      ToProcess.push(L);

    while (ToProcess.size() != 0) {
      auto *L = ToProcess.front();
      ToProcess.pop();
      for (auto *Child : *L)
        ToProcess.push(Child);

      assert(L->isLoopSimplifyForm());

      auto CT = getConvergenceToken(L->getHeader());
      SmallPtrSet<BasicBlock *, 8> RegionBlocks(L->block_begin(),
                                                L->block_end());
      SmallVector<BasicBlock *> LoopExits;
      L->getExitingBlocks(LoopExits);
      if (CT.has_value()) {
        for (auto *Exit : LoopExits) {
          auto N = findPathsToMatch(Exit, [&CT](const BasicBlock *block) {
            auto Token = getConvergenceToken(block);
            if (Token == std::nullopt)
              return false;
            return Token.value() == CT.value();
          });
          RegionBlocks.insert(N.begin(), N.end());
        }
      }

      auto RegionExits = findExitNodes(RegionBlocks);
      ConvergenceRegion *Region = new ConvergenceRegion(
          DT, LI, CT, L->getHeader(), std::move(RegionBlocks),
          std::move(RegionExits));

      auto It = LoopToRegion.find(L->getParentLoop());
      assert(It != LoopToRegion.end() || L->getParentLoop() == nullptr);
      Region->Parent = It != LoopToRegion.end() ? It->second : TopLevelRegion;
      Region->Parent->Children.push_back(Region);

      LoopToRegion.emplace(L, Region);
    }

    return ConvergenceRegionInfo(TopLevelRegion);
  }

private:
  DominatorTree &DT;
  LoopInfo &LI;
  Function &F;
};

ConvergenceRegionInfo getConvergenceRegions(Function &F, DominatorTree &DT,
                                            LoopInfo &LI) {
  ConvergenceRegionAnalyzer Analyzer(F, DT, LI);
  return Analyzer.analyze();
}

char ConvergenceRegionAnalysisWrapperPass::ID = 0;

ConvergenceRegionAnalysisWrapperPass::ConvergenceRegionAnalysisWrapperPass()
    : FunctionPass(ID) {}

bool ConvergenceRegionAnalysisWrapperPass::runOnFunction(Function &F) {
  DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  CRI = getConvergenceRegions(F, DT, LI);
  // Nothing was modified.
  return false;
}

ConvergenceRegionAnalysis::Result
ConvergenceRegionAnalysis::run(Function &F, FunctionAnalysisManager &AM) {
  Result CRI;
  auto &DT = AM.getResult<DominatorTreeAnalysis>(F);
  auto &LI = AM.getResult<LoopAnalysis>(F);
  CRI = getConvergenceRegions(F, DT, LI);
  return CRI;
}

AnalysisKey ConvergenceRegionAnalysis::Key;

} // namespace SPIRV
} // namespace llvm
