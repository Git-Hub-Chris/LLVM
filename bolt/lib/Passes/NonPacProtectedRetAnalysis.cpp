//===- bolt/Passes/NonPacProtectedRetAnalysis.cpp -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements a pass that looks for any AArch64 return instructions
// that may not be protected by PAuth authentication instructions when needed.
//
// When needed = the register used to return (almost always X30), is potentially
// written to between the AUThentication instruction and the RETurn instruction.
//
//===----------------------------------------------------------------------===//

#include "bolt/Passes/NonPacProtectedRetAnalysis.h"
#include "bolt/Core/ParallelUtilities.h"
#include "bolt/Passes/DataflowAnalysis.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/Format.h"

#define DEBUG_TYPE "bolt-nonpacprotectedret"

namespace llvm {
namespace bolt {

raw_ostream &operator<<(raw_ostream &OS, const MCInstInBBReference &Ref) {
  OS << "MCInstBBRef<";
  if (Ref.BB == nullptr)
    OS << "BB:(null)";
  else
    OS << "BB:" << Ref.BB->getName() << ":" << Ref.BBIndex;
  OS << ">";
  return OS;
}

raw_ostream &operator<<(raw_ostream &OS, const MCInstInBFReference &Ref) {
  OS << "MCInstBFRef<";
  if (Ref.BF == nullptr)
    OS << "BF:(null)";
  else
    OS << "BF:" << Ref.BF->getPrintName() << ":" << Ref.getOffset();
  OS << ">";
  return OS;
}

raw_ostream &operator<<(raw_ostream &OS, const MCInstReference &Ref) {
  switch (Ref.CurrentLocation) {
  case MCInstReference::_BinaryBasicBlock:
    OS << Ref.U.BBRef;
    return OS;
  case MCInstReference::_BinaryFunction:
    OS << Ref.U.BFRef;
    return OS;
  }
  llvm_unreachable("");
}

raw_ostream &operator<<(raw_ostream &OS,
                        const NonPacProtectedRetGadget &NPPRG) {
  OS << "pac-ret-gadget<";
  OS << "Ret:" << NPPRG.RetInst << ", ";
  OS << "Overwriting:[";
  for (auto Ref : NPPRG.OverwritingRetRegInst)
    OS << Ref << " ";
  OS << "]>";
  return OS;
}

// The security property that is checked is:
// When a register is used as the address to jump to in a return instruction,
// that register must either:
// (a) never be changed within this function, i.e. have the same value as when
//     the function started, or
// (b) the last write to the register must be by an authentication instruction.

// This property is checked by using data flow analysis to keep track of which
// registers have been written (def-ed), since last authenticated. Those are
// exactly the registers containing values that should not be trusted (as they
// could have changed since the last time they were authenticated). For pac-ret,
// any return instruction using such a register is a gadget to be reported. For
// PAuthABI, any indirect control flow using such a register should be reported?

// This security property is verified using a dataflow analysis.

// Furthermore, when producing a diagnostic for a found non-pac-ret protected
// return, the analysis also lists the last instructions that wrote to the
// register used in the return instruction.
// The total set of registers used in return instructions in a given function is
// small. It almost always is just `X30`.
// In order to reduce the memory consumption of storing this additional state
// during the dataflow analysis, this is computed by running the dataflow
// analysis twice:
// 1. In the first run, the dataflow analysis only keeps track of the security
//    property: i.e. which registers have been overwritten since the last
//    time they've been authenticated.
// 2. If the first run finds any return instructions using a
//    written-to-last-by-an-non-authenticating instruction, the data flow
//    analysis will be run a second time. The first run will return which
//    registers are used in the gadgets to be reported. This information is
//    used in the second run to also track with instructions last wrote to
//    those registers.

struct State {
  /// A BitVector containing the registers that have been clobbered, and
  /// not authenticated.
  BitVector NonAutClobRegs;
  /// A vector of sets, only used in the second data flow run.
  /// Each element in the vector represent one registers for which we
  /// track the set of last instructions that wrote to this register.
  /// For pac-ret analysis, the expectations is that almost all return
  /// instructions only use register `X30`, and therefore, this vector
  /// will have length 1 in the second run.
  std::vector<SmallPtrSet<const MCInst *, 4>> LastInstWritingReg;
  State() {}
  State(uint16_t NumRegs, uint16_t NumRegsToTrack)
      : NonAutClobRegs(NumRegs), LastInstWritingReg(NumRegsToTrack) {}
  State &operator|=(const State &StateIn) {
    NonAutClobRegs |= StateIn.NonAutClobRegs;
    for (unsigned I = 0; I < LastInstWritingReg.size(); ++I)
      for (const MCInst *J : StateIn.LastInstWritingReg[I])
        LastInstWritingReg[I].insert(J);
    return *this;
  }
  bool operator==(const State &RHS) const {
    return NonAutClobRegs == RHS.NonAutClobRegs &&
           LastInstWritingReg == RHS.LastInstWritingReg;
  }
  bool operator!=(const State &RHS) const { return !((*this) == RHS); }
};

static void printLastInsts(
    raw_ostream &OS,
    const std::vector<SmallPtrSet<const MCInst *, 4>> &LastInstWritingReg) {
  OS << "Insts: ";
  for (unsigned I = 0; I < LastInstWritingReg.size(); ++I) {
    auto Set = LastInstWritingReg[I];
    OS << "[" << I << "](";
    for (const MCInst *MCInstP : Set)
      OS << MCInstP << " ";
    OS << ")";
  }
}

raw_ostream &operator<<(raw_ostream &OS, const State &S) {
  OS << "pacret-state<";
  OS << "NonAutClobRegs: " << S.NonAutClobRegs;
  OS << "Insts: ";
  printLastInsts(OS, S.LastInstWritingReg);
  OS << ">";
  return OS;
}

class PacStatePrinter {
public:
  void print(raw_ostream &OS, const State &State) const;
  explicit PacStatePrinter(const BinaryContext &BC) : BC(BC) {}

private:
  const BinaryContext &BC;
};

void PacStatePrinter::print(raw_ostream &OS, const State &S) const {
  RegStatePrinter RegStatePrinter(BC);
  OS << "pacret-state<";
  OS << "NonAutClobRegs: ";
  RegStatePrinter.print(OS, S.NonAutClobRegs);
  OS << "Insts: ";
  printLastInsts(OS, S.LastInstWritingReg);
  OS << ">";
}

class PacRetAnalysis
    : public DataflowAnalysis<PacRetAnalysis, State, false /*Backward*/,
                              PacStatePrinter> {
  using Parent =
      DataflowAnalysis<PacRetAnalysis, State, false, PacStatePrinter>;
  friend Parent;

public:
  PacRetAnalysis(BinaryFunction &BF, MCPlusBuilder::AllocatorIdTy AllocId,
                 const std::vector<MCPhysReg> &RegsToTrackInstsFor)
      : Parent(BF, AllocId), NumRegs(BF.getBinaryContext().MRI->getNumRegs()),
        RegsToTrackInstsFor(RegsToTrackInstsFor),
        TrackingLastInsts(RegsToTrackInstsFor.size() != 0),
        Reg2StateIdx(RegsToTrackInstsFor.size() == 0
                         ? 0
                         : *std::max_element(RegsToTrackInstsFor.begin(),
                                             RegsToTrackInstsFor.end()) +
                               1,
                     -1) {
    for (unsigned I = 0; I < RegsToTrackInstsFor.size(); ++I)
      Reg2StateIdx[RegsToTrackInstsFor[I]] = I;
  }
  virtual ~PacRetAnalysis() {}

  void run() { Parent::run(); }

protected:
  const uint16_t NumRegs;
  /// RegToTrackInstsFor is the set of registers for which the dataflow analysis
  /// must compute which the last set of instructions writing to it are.
  const std::vector<MCPhysReg> RegsToTrackInstsFor;
  const bool TrackingLastInsts;
  /// Reg2StateIdx maps Register to the index in the vector used in State to
  /// track which instructions last wrote to this register.
  std::vector<uint16_t> Reg2StateIdx;

  bool trackReg(MCPhysReg Reg) {
    for (auto R : RegsToTrackInstsFor)
      if (R == Reg)
        return true;
    return false;
  }
  uint16_t reg2StateIdx(MCPhysReg Reg) const {
    assert(Reg < Reg2StateIdx.size());
    return Reg2StateIdx[Reg];
  }

  void preflight() {}

  State getStartingStateAtBB(const BinaryBasicBlock &BB) {
    return State(NumRegs, RegsToTrackInstsFor.size());
  }

  State getStartingStateAtPoint(const MCInst &Point) {
    return State(NumRegs, RegsToTrackInstsFor.size());
  }

  void doConfluence(State &StateOut, const State &StateIn) {
    PacStatePrinter P(BC);
    LLVM_DEBUG({
      dbgs() << " PacRetAnalysis::Confluence(\n";
      dbgs() << "   State 1: ";
      P.print(dbgs(), StateOut);
      dbgs() << "\n";
      dbgs() << "   State 2: ";
      P.print(dbgs(), StateIn);
      dbgs() << ")\n";
    });

    StateOut |= StateIn;

    LLVM_DEBUG({
      dbgs() << "   merged state: ";
      P.print(dbgs(), StateOut);
      dbgs() << "\n";
    });
  }

  State computeNext(const MCInst &Point, const State &Cur) {
    PacStatePrinter P(BC);
    LLVM_DEBUG({
      dbgs() << " PacRetAnalysis::ComputeNext(";
      BC.InstPrinter->printInst(&const_cast<MCInst &>(Point), 0, "", *BC.STI,
                                dbgs());
      dbgs() << ", ";
      P.print(dbgs(), Cur);
      dbgs() << ")\n";
    });

    State Next = Cur;
    BitVector Written = BitVector(NumRegs, false);
    BC.MIB->getWrittenRegs(Point, Written);
    Next.NonAutClobRegs |= Written;
    // Keep track of this instruction if it writes to any of the registers we
    // need to track that for:
    if (TrackingLastInsts) {
      for (auto WrittenReg : Written.set_bits()) {
        if (trackReg(WrittenReg)) {
          Next.LastInstWritingReg[reg2StateIdx(WrittenReg)] = {};
          Next.LastInstWritingReg[reg2StateIdx(WrittenReg)].insert(&Point);
        }
      }
    }

    MCPhysReg AutReg = BC.MIB->getAuthenticatedReg(Point);
    if (AutReg != BC.MIB->getNoRegister()) {
      Next.NonAutClobRegs.reset(
          BC.MIB->getAliases(AutReg, /*OnlySmaller=*/true));
      if (TrackingLastInsts && trackReg(AutReg))
        Next.LastInstWritingReg[reg2StateIdx(AutReg)] = {};
    }

    LLVM_DEBUG({
      dbgs() << "  .. result: (";
      P.print(dbgs(), Next);
      dbgs() << ")\n";
    });

    return Next;
  }

  StringRef getAnnotationName() const { return StringRef("PacRetAnalysis"); }

public:
  std::vector<MCInstReference>
  getLastClobberingInsts(const MCInst Ret, BinaryFunction &BF,
                         const BitVector &DirtyRawRegs) const {
    if (!TrackingLastInsts)
      return {};
    if (auto MaybeState = getStateAt(Ret)) {
      const State &S = *MaybeState;
      // Due to aliasing registers, multiple registers may have
      // been tracked.
      std::set<const MCInst *> LastWritingInsts;
      for (MCPhysReg TrackedReg : DirtyRawRegs.set_bits()) {
        for (const MCInst *LastInstWriting :
             S.LastInstWritingReg[reg2StateIdx(TrackedReg)])
          LastWritingInsts.insert(LastInstWriting);
      }
      std::vector<MCInstReference> Result;
      for (const MCInst *LastInstWriting : LastWritingInsts) {
        MCInstInBBReference Ref = MCInstInBBReference::get(LastInstWriting, BF);
        assert(Ref.BB != nullptr && "Expected Inst to be found");
        Result.push_back(MCInstReference(Ref));
      }
      return Result;
    }
    llvm_unreachable("Expected State to be present");
  }
};

SmallSet<MCPhysReg, 1> NonPacProtectedRetAnalysis::computeDfState(
    PacRetAnalysis &PRA, BinaryFunction &BF,
    MCPlusBuilder::AllocatorIdTy AllocatorId) {
  PRA.run();
  LLVM_DEBUG({
    dbgs() << " After PacRetAnalysis:\n";
    BF.dump();
  });
  // Now scan the CFG for non-authenticating return instructions that use an
  // overwritten, non-authenticated register as return address.
  SmallSet<MCPhysReg, 1> RetRegsWithGadgets;
  BinaryContext &BC = BF.getBinaryContext();
  for (BinaryBasicBlock &BB : BF) {
    for (int64_t I = BB.size() - 1; I >= 0; --I) {
      MCInst &Inst = BB.getInstructionAtIndex(I);
      if (BC.MIB->isReturn(Inst)) {
        MCPhysReg RetReg = BC.MIB->getRegUsedAsRetDest(Inst);
        LLVM_DEBUG({
          dbgs() << "  Found RET inst: ";
          BC.printInstruction(dbgs(), Inst);
          dbgs() << "    RetReg: " << BC.MRI->getName(RetReg)
                 << "; authenticatesReg: "
                 << BC.MIB->isAuthenticationOfReg(Inst, RetReg) << "\n";
        });
        if (BC.MIB->isAuthenticationOfReg(Inst, RetReg))
          break;
        BitVector DirtyRawRegs = PRA.getStateAt(Inst)->NonAutClobRegs;
        LLVM_DEBUG({
          dbgs() << "  DirtyRawRegs at Ret: ";
          RegStatePrinter RSP(BC);
          RSP.print(dbgs(), DirtyRawRegs);
          dbgs() << "\n";
        });
        DirtyRawRegs &= BC.MIB->getAliases(RetReg, /*OnlySmaller=*/true);
        LLVM_DEBUG({
          dbgs() << "  Intersection with RetReg: ";
          RegStatePrinter RSP(BC);
          RSP.print(dbgs(), DirtyRawRegs);
          dbgs() << "\n";
        });
        if (DirtyRawRegs.any()) {
          // This return instruction needs to be reported
          // First remove the annotation that the first, fast run of
          // the dataflow analysis may have added.
          if (BC.MIB->hasAnnotation(Inst, GadgetAnnotationIndex))
            BC.MIB->removeAnnotation(Inst, GadgetAnnotationIndex);
          BC.MIB->addAnnotation(
              Inst, GadgetAnnotationIndex,
              NonPacProtectedRetGadget(
                  MCInstInBBReference(&BB, I),
                  PRA.getLastClobberingInsts(Inst, BF, DirtyRawRegs)),
              AllocatorId);
          LLVM_DEBUG({
            dbgs() << "  Added gadget info annotation: ";
            BC.printInstruction(dbgs(), Inst, 0, &BF);
            dbgs() << "\n";
          });
          for (MCPhysReg RetRegWithGadget : DirtyRawRegs.set_bits())
            RetRegsWithGadgets.insert(RetRegWithGadget);
        }
      }
    }
  }
  return RetRegsWithGadgets;
}

void NonPacProtectedRetAnalysis::runOnFunction(
    BinaryFunction &BF, MCPlusBuilder::AllocatorIdTy AllocatorId) {
  LLVM_DEBUG({
    dbgs() << "Analyzing in function " << BF.getPrintName() << ", AllocatorId "
           << AllocatorId << "\n";
  });
  LLVM_DEBUG({ BF.dump(); });

  if (BF.hasCFG()) {
    PacRetAnalysis PRA(BF, AllocatorId, {});
    SmallSet<MCPhysReg, 1> RetRegsWithGadgets =
        computeDfState(PRA, BF, AllocatorId);
    if (!RetRegsWithGadgets.empty()) {
      // Redo the analysis, but now also track which instructions last wrote
      // to any of the registers in RetRegsWithGadgets, so that better
      // diagnostics can be produced.
      std::vector<MCPhysReg> RegsToTrack;
      for (MCPhysReg R : RetRegsWithGadgets)
        RegsToTrack.push_back(R);
      PacRetAnalysis PRWIA(BF, AllocatorId, RegsToTrack);
      SmallSet<MCPhysReg, 1> RetRegsWithGadgets =
          computeDfState(PRWIA, BF, AllocatorId);
    }
  }
}

void printBB(const BinaryContext &BC, const BinaryBasicBlock *BB,
             size_t StartIndex = 0, size_t EndIndex = -1) {
  if (EndIndex == (size_t)-1)
    EndIndex = BB->size() - 1;
  const BinaryFunction *BF = BB->getFunction();
  for (unsigned I = StartIndex; I <= EndIndex; ++I) {
    uint64_t Address = BB->getOffset() + BF->getAddress() + 4 * I;
    const MCInst &Inst = BB->getInstructionAtIndex(I);
    if (BC.MIB->isCFI(Inst))
      continue;
    BC.printInstruction(outs(), Inst, Address, BF);
  }
}

void reportFoundGadgetInSingleBBSingleOverwInst(const BinaryContext &BC,
                                                const MCInstReference OverwInst,
                                                const MCInstReference RetInst) {
  BinaryBasicBlock *BB = RetInst.getBasicBlock();
  assert(OverwInst.CurrentLocation == MCInstReference::_BinaryBasicBlock);
  assert(RetInst.CurrentLocation == MCInstReference::_BinaryBasicBlock);
  MCInstInBBReference OverwInstBB = OverwInst.U.BBRef;
  if (BB == OverwInstBB.BB) {
    // overwriting inst and ret instruction are in the same basic block.
    assert(OverwInstBB.BBIndex < RetInst.U.BBRef.BBIndex);
    outs() << "  This happens in the following basic block:\n";
    printBB(BC, BB);
  }
}

void reportFoundGadget(const BinaryContext &BC, const MCInst &Inst,
                       unsigned int GadgetAnnotationIndex) {
  auto NPPRG = BC.MIB->getAnnotationAs<NonPacProtectedRetGadget>(
      Inst, GadgetAnnotationIndex);
  MCInstReference RetInst = NPPRG.RetInst;
  BinaryFunction *BF = RetInst.getFunction();
  BinaryBasicBlock *BB = RetInst.getBasicBlock();

  outs() << "\nGS-PACRET: " << "non-protected ret found in function "
         << BF->getPrintName();
  if (BB)
    outs() << ", basic block " << BB->getName();
  outs() << ", at address " << llvm::format("%x", RetInst.getAddress()) << "\n";
  outs() << "  The return instruction is ";
  BC.printInstruction(outs(), RetInst, RetInst.getAddress(), BF);
  outs() << "  The " << NPPRG.OverwritingRetRegInst.size()
         << " instructions that write to the return register after any "
            "authentication are:\n";
  // Sort by address to ensure output is deterministic.
  std::sort(std::begin(NPPRG.OverwritingRetRegInst),
            std::end(NPPRG.OverwritingRetRegInst),
            [](const MCInstReference &A, const MCInstReference &B) {
              return A.getAddress() < B.getAddress();
            });
  for (unsigned I = 0; I < NPPRG.OverwritingRetRegInst.size(); ++I) {
    MCInstReference InstRef = NPPRG.OverwritingRetRegInst[I];
    outs() << "  " << (I + 1) << ". ";
    BC.printInstruction(outs(), InstRef, InstRef.getAddress(), BF);
  };
  LLVM_DEBUG({
    dbgs() << "  .. OverWritingRetRegInst:\n";
    for (MCInstReference Ref : NPPRG.OverwritingRetRegInst) {
      dbgs() << "    " << Ref << "\n";
    }
  });
  if (NPPRG.OverwritingRetRegInst.size() == 1) {
    const MCInstReference OverwInst = NPPRG.OverwritingRetRegInst[0];
    assert(OverwInst.CurrentLocation == MCInstReference::_BinaryBasicBlock);
    reportFoundGadgetInSingleBBSingleOverwInst(BC, OverwInst, RetInst);
  }
}

Error NonPacProtectedRetAnalysis::runOnFunctions(BinaryContext &BC) {
  GadgetAnnotationIndex = BC.MIB->getOrCreateAnnotationIndex("pacret-gadget");

  ParallelUtilities::WorkFuncWithAllocTy WorkFun =
      [&](BinaryFunction &BF, MCPlusBuilder::AllocatorIdTy AllocatorId) {
        runOnFunction(BF, AllocatorId);
      };

  ParallelUtilities::PredicateTy SkipFunc = [&](const BinaryFunction &BF) {
    return false; // BF.shouldPreserveNops();
  };

  ParallelUtilities::runOnEachFunctionWithUniqueAllocId(
      BC, ParallelUtilities::SchedulingPolicy::SP_INST_LINEAR, WorkFun,
      SkipFunc, "NonPacProtectedRetAnalysis");

  for (BinaryFunction *BF : BC.getAllBinaryFunctions())
    if (BF->hasCFG()) {
      for (BinaryBasicBlock &BB : *BF) {
        for (int64_t I = BB.size() - 1; I >= 0; --I) {
          MCInst &Inst = BB.getInstructionAtIndex(I);
          if (BC.MIB->hasAnnotation(Inst, GadgetAnnotationIndex)) {
            reportFoundGadget(BC, Inst, GadgetAnnotationIndex);
          }
        }
      }
    }
  return Error::success();
}

} // namespace bolt
} // namespace llvm