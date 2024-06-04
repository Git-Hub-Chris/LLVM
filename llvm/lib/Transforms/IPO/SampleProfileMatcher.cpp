//===- SampleProfileMatcher.cpp - Sampling-based Stale Profile Matcher ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the SampleProfileMatcher used for stale
// profile matching.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/IPO/SampleProfileMatcher.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/MDBuilder.h"

using namespace llvm;
using namespace sampleprof;

#define DEBUG_TYPE "sample-profile-matcher"

static cl::opt<bool> SalvageUnusedProfile(
    "salvage-unused-profile", cl::Hidden, cl::init(false),
    cl::desc(
        "Salvage unused profile by matching new functions on call graph."));

static cl::opt<unsigned> FuncProfileSimilarityThreshold(
    "func-profile-similarity-threshold", cl::Hidden, cl::init(80),
    cl::desc("The profile matches the function if their similarity is above "
             "the given number(percentage)."));

static cl::opt<unsigned> MinFuncCountForCGMatching(
    "min-func-count-for-cg-matching", cl::Hidden, cl::init(5),
    cl::desc("The minimum number of basic blocks required for a function to "
             "run stale profile call graph matching."));

static cl::opt<unsigned> MinCallCountForCGMatching(
    "min-call-count-for-cg-matching", cl::Hidden, cl::init(3),
    cl::desc("The minimum number of call anchors required for a function to "
             "run stale profile call graph matching."));

extern cl::opt<bool> SalvageStaleProfile;
extern cl::opt<bool> PersistProfileStaleness;
extern cl::opt<bool> ReportProfileStaleness;

void SampleProfileMatcher::findIRAnchors(const Function &F,
                                         AnchorMap &IRAnchors) const {
  // For inlined code, recover the original callsite and callee by finding the
  // top-level inline frame. e.g. For frame stack "main:1 @ foo:2 @ bar:3", the
  // top-level frame is "main:1", the callsite is "1" and the callee is "foo".
  auto FindTopLevelInlinedCallsite = [](const DILocation *DIL) {
    assert((DIL && DIL->getInlinedAt()) && "No inlined callsite");
    const DILocation *PrevDIL = nullptr;
    do {
      PrevDIL = DIL;
      DIL = DIL->getInlinedAt();
    } while (DIL->getInlinedAt());

    LineLocation Callsite = FunctionSamples::getCallSiteIdentifier(
        DIL, FunctionSamples::ProfileIsFS);
    StringRef CalleeName = PrevDIL->getSubprogramLinkageName();
    return std::make_pair(Callsite, FunctionId(CalleeName));
  };

  auto GetCanonicalCalleeName = [](const CallBase *CB) {
    StringRef CalleeName = UnknownIndirectCallee;
    if (Function *Callee = CB->getCalledFunction())
      CalleeName = FunctionSamples::getCanonicalFnName(Callee->getName());
    return CalleeName;
  };

  // Extract profile matching anchors in the IR.
  for (auto &BB : F) {
    for (auto &I : BB) {
      DILocation *DIL = I.getDebugLoc();
      if (!DIL)
        continue;

      if (FunctionSamples::ProfileIsProbeBased) {
        if (auto Probe = extractProbe(I)) {
          // Flatten inlined IR for the matching.
          if (DIL->getInlinedAt()) {
            IRAnchors.emplace(FindTopLevelInlinedCallsite(DIL));
          } else {
            // Use empty StringRef for basic block probe.
            StringRef CalleeName;
            if (const auto *CB = dyn_cast<CallBase>(&I)) {
              // Skip the probe inst whose callee name is "llvm.pseudoprobe".
              if (!isa<IntrinsicInst>(&I))
                CalleeName = GetCanonicalCalleeName(CB);
            }
            LineLocation Loc = LineLocation(Probe->Id, 0);
            IRAnchors.emplace(Loc, FunctionId(CalleeName));
          }
        }
      } else {
        // TODO: For line-number based profile(AutoFDO), currently only support
        // find callsite anchors. In future, we need to parse all the non-call
        // instructions to extract the line locations for profile matching.
        if (!isa<CallBase>(&I) || isa<IntrinsicInst>(&I))
          continue;

        if (DIL->getInlinedAt()) {
          IRAnchors.emplace(FindTopLevelInlinedCallsite(DIL));
        } else {
          LineLocation Callsite = FunctionSamples::getCallSiteIdentifier(
              DIL, FunctionSamples::ProfileIsFS);
          StringRef CalleeName = GetCanonicalCalleeName(dyn_cast<CallBase>(&I));
          IRAnchors.emplace(Callsite, FunctionId(CalleeName));
        }
      }
    }
  }
}

void SampleProfileMatcher::findProfileAnchors(const FunctionSamples &FS,
                                              AnchorMap &ProfileAnchors) const {
  auto isInvalidLineOffset = [](uint32_t LineOffset) {
    return LineOffset & 0x8000;
  };

  auto InsertAnchor = [](const LineLocation &Loc, const FunctionId &CalleeName,
                         AnchorMap &ProfileAnchors) {
    auto Ret = ProfileAnchors.try_emplace(Loc, CalleeName);
    if (!Ret.second) {
      // For multiple callees, which indicates it's an indirect call, we use a
      // dummy name(UnknownIndirectCallee) as the indrect callee name.
      Ret.first->second = FunctionId(UnknownIndirectCallee);
    }
  };

  for (const auto &I : FS.getBodySamples()) {
    const LineLocation &Loc = I.first;
    if (isInvalidLineOffset(Loc.LineOffset))
      continue;
    for (const auto &C : I.second.getCallTargets())
      InsertAnchor(Loc, C.first, ProfileAnchors);
  }

  for (const auto &I : FS.getCallsiteSamples()) {
    const LineLocation &Loc = I.first;
    if (isInvalidLineOffset(Loc.LineOffset))
      continue;
    for (const auto &C : I.second)
      InsertAnchor(Loc, C.first, ProfileAnchors);
  }
}

LocToLocMap SampleProfileMatcher::longestCommonSequence(
    const AnchorList &AnchorList1, const AnchorList &AnchorList2) const {
  int32_t Size1 = AnchorList1.size(), Size2 = AnchorList2.size(),
          MaxDepth = Size1 + Size2;
  auto Index = [&](int32_t I) { return I + MaxDepth; };

  LocToLocMap EqualLocations;
  if (MaxDepth == 0)
    return EqualLocations;

  // Backtrack the SES result.
  auto Backtrack = [&](const std::vector<std::vector<int32_t>> &Trace,
                       const AnchorList &AnchorList1,
                       const AnchorList &AnchorList2,
                       LocToLocMap &EqualLocations) {
    int32_t X = Size1, Y = Size2;
    for (int32_t Depth = Trace.size() - 1; X > 0 || Y > 0; Depth--) {
      const auto &P = Trace[Depth];
      int32_t K = X - Y;
      int32_t PrevK = K;
      if (K == -Depth || (K != Depth && P[Index(K - 1)] < P[Index(K + 1)]))
        PrevK = K + 1;
      else
        PrevK = K - 1;

      int32_t PrevX = P[Index(PrevK)];
      int32_t PrevY = PrevX - PrevK;
      while (X > PrevX && Y > PrevY) {
        X--;
        Y--;
        EqualLocations.insert({AnchorList1[X].first, AnchorList2[Y].first});
      }

      if (Depth == 0)
        break;

      if (Y == PrevY)
        X--;
      else if (X == PrevX)
        Y--;
      X = PrevX;
      Y = PrevY;
    }
  };

  // The greedy LCS/SES algorithm.

  // An array contains the endpoints of the furthest reaching D-paths.
  std::vector<int32_t> V(2 * MaxDepth + 1, -1);
  V[Index(1)] = 0;
  // Trace is used to backtrack the SES result.
  std::vector<std::vector<int32_t>> Trace;
  for (int32_t Depth = 0; Depth <= MaxDepth; Depth++) {
    Trace.push_back(V);
    for (int32_t K = -Depth; K <= Depth; K += 2) {
      int32_t X = 0, Y = 0;
      if (K == -Depth || (K != Depth && V[Index(K - 1)] < V[Index(K + 1)]))
        X = V[Index(K + 1)];
      else
        X = V[Index(K - 1)] + 1;
      Y = X - K;
      while (X < Size1 && Y < Size2 &&
             AnchorList1[X].second == AnchorList2[Y].second)
        X++, Y++;

      V[Index(K)] = X;

      if (X >= Size1 && Y >= Size2) {
        // Length of an SES is D.
        Backtrack(Trace, AnchorList1, AnchorList2, EqualLocations);
        return EqualLocations;
      }
    }
  }
  // Length of an SES is greater than MaxDepth.
  return EqualLocations;
}

void SampleProfileMatcher::matchNonCallsiteLocs(
    const LocToLocMap &MatchedAnchors, const AnchorMap &IRAnchors,
    LocToLocMap &IRToProfileLocationMap) {
  auto InsertMatching = [&](const LineLocation &From, const LineLocation &To) {
    // Skip the unchanged location mapping to save memory.
    if (From != To)
      IRToProfileLocationMap.insert({From, To});
  };

  // Use function's beginning location as the initial anchor.
  int32_t LocationDelta = 0;
  SmallVector<LineLocation> LastMatchedNonAnchors;
  for (const auto &IR : IRAnchors) {
    const auto &Loc = IR.first;
    bool IsMatchedAnchor = false;
    // Match the anchor location in lexical order.
    auto R = MatchedAnchors.find(Loc);
    if (R != MatchedAnchors.end()) {
      const auto &Candidate = R->second;
      InsertMatching(Loc, Candidate);
      LLVM_DEBUG(dbgs() << "Callsite with callee:" << IR.second.stringRef()
                        << " is matched from " << Loc << " to " << Candidate
                        << "\n");
      LocationDelta = Candidate.LineOffset - Loc.LineOffset;

      // Match backwards for non-anchor locations.
      // The locations in LastMatchedNonAnchors have been matched forwards
      // based on the previous anchor, spilt it evenly and overwrite the
      // second half based on the current anchor.
      for (size_t I = (LastMatchedNonAnchors.size() + 1) / 2;
           I < LastMatchedNonAnchors.size(); I++) {
        const auto &L = LastMatchedNonAnchors[I];
        uint32_t CandidateLineOffset = L.LineOffset + LocationDelta;
        LineLocation Candidate(CandidateLineOffset, L.Discriminator);
        InsertMatching(L, Candidate);
        LLVM_DEBUG(dbgs() << "Location is rematched backwards from " << L
                          << " to " << Candidate << "\n");
      }

      IsMatchedAnchor = true;
      LastMatchedNonAnchors.clear();
    }

    // Match forwards for non-anchor locations.
    if (!IsMatchedAnchor) {
      uint32_t CandidateLineOffset = Loc.LineOffset + LocationDelta;
      LineLocation Candidate(CandidateLineOffset, Loc.Discriminator);
      InsertMatching(Loc, Candidate);
      LLVM_DEBUG(dbgs() << "Location is matched from " << Loc << " to "
                        << Candidate << "\n");
      LastMatchedNonAnchors.emplace_back(Loc);
    }
  }
}

// Filter the non-call locations from IRAnchors and ProfileAnchors and write
// them into a list for random access later.
void SampleProfileMatcher::getFilteredAnchorList(
    const AnchorMap &IRAnchors, const AnchorMap &ProfileAnchors,
    AnchorList &FilteredIRAnchorsList, AnchorList &FilteredProfileAnchorList) {
  for (const auto &I : IRAnchors) {
    if (I.second.stringRef().empty())
      continue;
    FilteredIRAnchorsList.emplace_back(I);
  }

  for (const auto &I : ProfileAnchors)
    FilteredProfileAnchorList.emplace_back(I);
}

// Call target name anchor based profile fuzzy matching.
// Input:
// For IR locations, the anchor is the callee name of direct callsite; For
// profile locations, it's the call target name for BodySamples or inlinee's
// profile name for CallsiteSamples.
// Matching heuristic:
// First match all the anchors using the diff algorithm, then split the
// non-anchor locations between the two anchors evenly, first half are matched
// based on the start anchor, second half are matched based on the end anchor.
// For example, given:
// IR locations:      [1, 2(foo), 3, 5, 6(bar), 7]
// Profile locations: [1, 2, 3(foo), 4, 7, 8(bar), 9]
// The matching gives:
//   [1,    2(foo), 3,  5,  6(bar), 7]
//    |     |       |   |     |     |
//   [1, 2, 3(foo), 4,  7,  8(bar), 9]
// The output mapping: [2->3, 3->4, 5->7, 6->8, 7->9].
void SampleProfileMatcher::runStaleProfileMatching(
    const Function &F, const AnchorMap &IRAnchors,
    const AnchorMap &ProfileAnchors, LocToLocMap &IRToProfileLocationMap) {
  LLVM_DEBUG(dbgs() << "Run stale profile matching for " << F.getName()
                    << "\n");
  assert(IRToProfileLocationMap.empty() &&
         "Run stale profile matching only once per function");

  AnchorList FilteredProfileAnchorList;
  AnchorList FilteredIRAnchorsList;
  getFilteredAnchorList(IRAnchors, ProfileAnchors, FilteredIRAnchorsList,
                        FilteredProfileAnchorList);

  if (FilteredIRAnchorsList.empty() || FilteredProfileAnchorList.empty())
    return;

  // Match the callsite anchors by finding the longest common subsequence
  // between IR and profile. Note that we need to use IR anchor as base(A side)
  // to align with the order of IRToProfileLocationMap.
  LocToLocMap MatchedAnchors =
      longestCommonSequence(FilteredIRAnchorsList, FilteredProfileAnchorList);

  // Match the non-callsite locations and write the result to
  // IRToProfileLocationMap.
  matchNonCallsiteLocs(MatchedAnchors, IRAnchors, IRToProfileLocationMap);
}

void SampleProfileMatcher::runCFGMatching(Function &F) {
  // We need to use flattened function samples for matching.
  // Unlike IR, which includes all callsites from the source code, the callsites
  // in profile only show up when they are hit by samples, i,e. the profile
  // callsites in one context may differ from those in another context. To get
  // the maximum number of callsites, we merge the function profiles from all
  // contexts, aka, the flattened profile to find profile anchors.
  const auto *FSFlattened = getFlattenedSamplesFor(F);
  if (!FSFlattened)
    return;

  // Anchors for IR. It's a map from IR location to callee name, callee name is
  // empty for non-call instruction and use a dummy name(UnknownIndirectCallee)
  // for unknown indrect callee name.
  AnchorMap IRAnchors;
  findIRAnchors(F, IRAnchors);
  // Anchors for profile. It's a map from callsite location to a set of callee
  // name.
  AnchorMap ProfileAnchors;
  findProfileAnchors(*FSFlattened, ProfileAnchors);

  // Compute the callsite match states for profile staleness report.
  if (ReportProfileStaleness || PersistProfileStaleness)
    recordCallsiteMatchStates(F, IRAnchors, ProfileAnchors, nullptr);

  // For probe-based profiles, run matching only when the current profile is not
  // valid.
  if (SalvageStaleProfile && (!FunctionSamples::ProfileIsProbeBased ||
                              !ProbeManager->profileIsValid(F, *FSFlattened))) {
    // For imported functions, the checksum metadata(pseudo_probe_desc) are
    // dropped, so we leverage function attribute(profile-checksum-mismatch) to
    // transfer the info: add the attribute during pre-link phase and check it
    // during post-link phase(see "profileIsValid").
    if (FunctionSamples::ProfileIsProbeBased &&
        LTOPhase == ThinOrFullLTOPhase::ThinLTOPreLink)
      F.addFnAttr("profile-checksum-mismatch");

    // The matching result will be saved to IRToProfileLocationMap, create a
    // new map for each function.
    auto &IRToProfileLocationMap = getIRToProfileLocationMap(F);
    runStaleProfileMatching(F, IRAnchors, ProfileAnchors,
                            IRToProfileLocationMap);
    // Find and update callsite match states after matching.
    if (ReportProfileStaleness || PersistProfileStaleness)
      recordCallsiteMatchStates(F, IRAnchors, ProfileAnchors,
                                &IRToProfileLocationMap);
  }
}

void SampleProfileMatcher::recordCallsiteMatchStates(
    const Function &F, const AnchorMap &IRAnchors,
    const AnchorMap &ProfileAnchors,
    const LocToLocMap *IRToProfileLocationMap) {
  bool IsPostMatch = IRToProfileLocationMap != nullptr;
  auto &CallsiteMatchStates =
      FuncCallsiteMatchStates[FunctionSamples::getCanonicalFnName(F.getName())];

  auto MapIRLocToProfileLoc = [&](const LineLocation &IRLoc) {
    // IRToProfileLocationMap is null in pre-match phrase.
    if (!IRToProfileLocationMap)
      return IRLoc;
    const auto &ProfileLoc = IRToProfileLocationMap->find(IRLoc);
    if (ProfileLoc != IRToProfileLocationMap->end())
      return ProfileLoc->second;
    else
      return IRLoc;
  };

  for (const auto &I : IRAnchors) {
    // After fuzzy profile matching, use the matching result to remap the
    // current IR callsite.
    const auto &ProfileLoc = MapIRLocToProfileLoc(I.first);
    const auto &IRCalleeId = I.second;
    const auto &It = ProfileAnchors.find(ProfileLoc);
    if (It == ProfileAnchors.end())
      continue;
    const auto &ProfCalleeId = It->second;
    if (IRCalleeId == ProfCalleeId) {
      auto It = CallsiteMatchStates.find(ProfileLoc);
      if (It == CallsiteMatchStates.end())
        CallsiteMatchStates.emplace(ProfileLoc, MatchState::InitialMatch);
      else if (IsPostMatch) {
        if (It->second == MatchState::InitialMatch)
          It->second = MatchState::UnchangedMatch;
        else if (It->second == MatchState::InitialMismatch)
          It->second = MatchState::RecoveredMismatch;
      }
    }
  }

  // Check if there are any callsites in the profile that does not match to any
  // IR callsites.
  for (const auto &I : ProfileAnchors) {
    const auto &Loc = I.first;
    assert(!I.second.stringRef().empty() && "Callees should not be empty");
    auto It = CallsiteMatchStates.find(Loc);
    if (It == CallsiteMatchStates.end())
      CallsiteMatchStates.emplace(Loc, MatchState::InitialMismatch);
    else if (IsPostMatch) {
      // Update the state if it's not matched(UnchangedMatch or
      // RecoveredMismatch).
      if (It->second == MatchState::InitialMismatch)
        It->second = MatchState::UnchangedMismatch;
      else if (It->second == MatchState::InitialMatch)
        It->second = MatchState::RemovedMatch;
    }
  }
}

void SampleProfileMatcher::countMismatchedFuncSamples(const FunctionSamples &FS,
                                                      bool IsTopLevel) {
  const auto *FuncDesc = ProbeManager->getDesc(FS.getGUID());
  // Skip the function that is external or renamed.
  if (!FuncDesc)
    return;

  if (ProbeManager->profileIsHashMismatched(*FuncDesc, FS)) {
    if (IsTopLevel)
      NumStaleProfileFunc++;
    // Given currently all probe ids are after block probe ids, once the
    // checksum is mismatched, it's likely all the callites are mismatched and
    // dropped. We conservatively count all the samples as mismatched and stop
    // counting the inlinees' profiles.
    MismatchedFunctionSamples += FS.getTotalSamples();
    return;
  }

  // Even the current-level function checksum is matched, it's possible that the
  // nested inlinees' checksums are mismatched that affect the inlinee's sample
  // loading, we need to go deeper to check the inlinees' function samples.
  // Similarly, count all the samples as mismatched if the inlinee's checksum is
  // mismatched using this recursive function.
  for (const auto &I : FS.getCallsiteSamples())
    for (const auto &CS : I.second)
      countMismatchedFuncSamples(CS.second, false);
}

void SampleProfileMatcher::countMismatchedCallsiteSamples(
    const FunctionSamples &FS) {
  auto It = FuncCallsiteMatchStates.find(FS.getFuncName());
  // Skip it if no mismatched callsite or this is an external function.
  if (It == FuncCallsiteMatchStates.end() || It->second.empty())
    return;
  const auto &CallsiteMatchStates = It->second;

  auto findMatchState = [&](const LineLocation &Loc) {
    auto It = CallsiteMatchStates.find(Loc);
    if (It == CallsiteMatchStates.end())
      return MatchState::Unknown;
    return It->second;
  };

  auto AttributeMismatchedSamples = [&](const enum MatchState &State,
                                        uint64_t Samples) {
    if (isMismatchState(State))
      MismatchedCallsiteSamples += Samples;
    else if (State == MatchState::RecoveredMismatch)
      RecoveredCallsiteSamples += Samples;
  };

  // The non-inlined callsites are saved in the body samples of function
  // profile, go through it to count the non-inlined callsite samples.
  for (const auto &I : FS.getBodySamples())
    AttributeMismatchedSamples(findMatchState(I.first), I.second.getSamples());

  // Count the inlined callsite samples.
  for (const auto &I : FS.getCallsiteSamples()) {
    auto State = findMatchState(I.first);
    uint64_t CallsiteSamples = 0;
    for (const auto &CS : I.second)
      CallsiteSamples += CS.second.getTotalSamples();
    AttributeMismatchedSamples(State, CallsiteSamples);

    if (isMismatchState(State))
      continue;

    // When the current level of inlined call site matches the profiled call
    // site, we need to go deeper along the inline tree to count mismatches from
    // lower level inlinees.
    for (const auto &CS : I.second)
      countMismatchedCallsiteSamples(CS.second);
  }
}

void SampleProfileMatcher::countMismatchCallsites(const FunctionSamples &FS) {
  auto It = FuncCallsiteMatchStates.find(FS.getFuncName());
  // Skip it if no mismatched callsite or this is an external function.
  if (It == FuncCallsiteMatchStates.end() || It->second.empty())
    return;
  const auto &MatchStates = It->second;
  [[maybe_unused]] bool OnInitialState =
      isInitialState(MatchStates.begin()->second);
  for (const auto &I : MatchStates) {
    TotalProfiledCallsites++;
    assert(
        (OnInitialState ? isInitialState(I.second) : isFinalState(I.second)) &&
        "Profile matching state is inconsistent");

    if (isMismatchState(I.second))
      NumMismatchedCallsites++;
    else if (I.second == MatchState::RecoveredMismatch)
      NumRecoveredCallsites++;
  }
}

void SampleProfileMatcher::computeAndReportProfileStaleness() {
  if (!ReportProfileStaleness && !PersistProfileStaleness)
    return;

  // Count profile mismatches for profile staleness report.
  for (const auto &F : M) {
    if (skipProfileForFunction(F))
      continue;
    // As the stats will be merged by linker, skip reporting the metrics for
    // imported functions to avoid repeated counting.
    if (GlobalValue::isAvailableExternallyLinkage(F.getLinkage()))
      continue;
    const auto *FS = Reader.getSamplesFor(F);
    if (!FS)
      continue;
    TotalProfiledFunc++;
    TotalFunctionSamples += FS->getTotalSamples();

    // Checksum mismatch is only used in pseudo-probe mode.
    if (FunctionSamples::ProfileIsProbeBased)
      countMismatchedFuncSamples(*FS, true);

    // Count mismatches and samples for calliste.
    countMismatchCallsites(*FS);
    countMismatchedCallsiteSamples(*FS);
  }

  if (ReportProfileStaleness) {
    if (FunctionSamples::ProfileIsProbeBased) {
      errs() << "(" << NumStaleProfileFunc << "/" << TotalProfiledFunc
             << ") of functions' profile are invalid and ("
             << MismatchedFunctionSamples << "/" << TotalFunctionSamples
             << ") of samples are discarded due to function hash mismatch.\n";
    }
    errs() << "(" << (NumMismatchedCallsites + NumRecoveredCallsites) << "/"
           << TotalProfiledCallsites
           << ") of callsites' profile are invalid and ("
           << (MismatchedCallsiteSamples + RecoveredCallsiteSamples) << "/"
           << TotalFunctionSamples
           << ") of samples are discarded due to callsite location mismatch.\n";
    errs() << "(" << NumRecoveredCallsites << "/"
           << (NumRecoveredCallsites + NumMismatchedCallsites)
           << ") of callsites and (" << RecoveredCallsiteSamples << "/"
           << (RecoveredCallsiteSamples + MismatchedCallsiteSamples)
           << ") of samples are recovered by stale profile matching.\n";
  }

  if (PersistProfileStaleness) {
    LLVMContext &Ctx = M.getContext();
    MDBuilder MDB(Ctx);

    SmallVector<std::pair<StringRef, uint64_t>> ProfStatsVec;
    if (FunctionSamples::ProfileIsProbeBased) {
      ProfStatsVec.emplace_back("NumStaleProfileFunc", NumStaleProfileFunc);
      ProfStatsVec.emplace_back("TotalProfiledFunc", TotalProfiledFunc);
      ProfStatsVec.emplace_back("MismatchedFunctionSamples",
                                MismatchedFunctionSamples);
      ProfStatsVec.emplace_back("TotalFunctionSamples", TotalFunctionSamples);
    }

    ProfStatsVec.emplace_back("NumMismatchedCallsites", NumMismatchedCallsites);
    ProfStatsVec.emplace_back("NumRecoveredCallsites", NumRecoveredCallsites);
    ProfStatsVec.emplace_back("TotalProfiledCallsites", TotalProfiledCallsites);
    ProfStatsVec.emplace_back("MismatchedCallsiteSamples",
                              MismatchedCallsiteSamples);
    ProfStatsVec.emplace_back("RecoveredCallsiteSamples",
                              RecoveredCallsiteSamples);

    auto *MD = MDB.createLLVMStats(ProfStatsVec);
    auto *NMD = M.getOrInsertNamedMetadata("llvm.stats");
    NMD->addOperand(MD);
  }
}

void SampleProfileMatcher::findNewIRFunctions(
    StringMap<Function *> &NewIRFunctions) {
  // TODO: Support MD5 profile.
  if (FunctionSamples::UseMD5)
    return;
  StringSet<> NamesInProfile;
  if (auto NameTable = Reader.getNameTable()) {
    for (auto Name : *NameTable)
      NamesInProfile.insert(Name.stringRef());
  }

  for (auto &F : M) {
    // Skip declarations, as even if the function can be recognized renamed, we
    // have nothing to do with it.
    if (F.isDeclaration())
      continue;

    StringRef CanonFName = FunctionSamples::getCanonicalFnName(F.getName());
    const auto *FS = getFlattenedSamplesFor(F);
    if (FS)
      continue;

    // For extended binary, functions are fully inlined may not be loaded in the
    // top-level profile, so check the NameTable which has the all symbol names
    // in profile.
    if (NamesInProfile.count(CanonFName))
      continue;

    // For extended binary, non-profiled function symbols are in the profile
    // symbol list table.
    if (PSL && PSL->contains(CanonFName))
      continue;

    LLVM_DEBUG(dbgs() << "Function " << CanonFName
                      << " is not in profile or symbol list table.\n");
    NewIRFunctions[CanonFName] = &F;
  }
}

std::vector<Function *> *SampleProfileMatcher::findNewIRCallees(
    Function &Func, const StringMap<Function *> &NewIRFunctions) {
  auto R = FuncToNewCalleesMap.try_emplace(&Func, std::vector<Function *>());
  std::vector<Function *> &IRCalleesToMatch = R.first->second;
  // Skip the lookup if it's in the cache.
  if (!R.second)
    return &IRCalleesToMatch;

  for (auto &BB : Func) {
    for (auto &I : BB) {
      const auto *CB = dyn_cast<CallBase>(&I);
      if (!CB || isa<IntrinsicInst>(&I))
        continue;
      Function *Callee = CB->getCalledFunction();
      if (!Callee || Callee->isDeclaration())
        continue;
      StringRef CalleeName =
          FunctionSamples::getCanonicalFnName(Callee->getName());
      if (NewIRFunctions.count(CalleeName))
        IRCalleesToMatch.push_back(Callee);
    }
  }
  return &IRCalleesToMatch;
}

Function *SampleProfileMatcher::findFuncByProfileName(
    const FunctionId &ProfileName) const {
  auto F = SymbolMap->find(ProfileName);
  if (F != SymbolMap->end())
    return F->second;

  // Find in new matched function map.
  auto NewF = ProfileNameToFuncMap.find(ProfileName);
  if (NewF != ProfileNameToFuncMap.end())
    return NewF->second;
  return nullptr;
}

bool SampleProfileMatcher::functionMatchesProfileHelper(
    const Function &IRFunc, const FunctionId &ProfFunc) {
  // The value is in the range [0, 1]. The bigger the value is, the more similar
  // two sequences are.
  float Similarity = 0.0;

  const auto *FSFlattened = getFlattenedSamplesFor(ProfFunc);
  assert(FSFlattened && "Flattened profile sample is null");
  // Similarity check may not be reliable if the function is tiny, we use the
  // number of basic block as a proxy for the function complexity and skip the
  // matching if it's too small.
  if (IRFunc.size() < MinFuncCountForCGMatching ||
      FSFlattened->getBodySamples().size() < MinFuncCountForCGMatching)
    return false;

  // For probe-based function, we first trust the checksum info. If the checksum
  // doesn't match, we continue checking for similarity.
  if (FunctionSamples::ProfileIsProbeBased) {
    const auto *FuncDesc = ProbeManager->getDesc(IRFunc);
    if (FuncDesc &&
        !ProbeManager->profileIsHashMismatched(*FuncDesc, *FSFlattened)) {
      LLVM_DEBUG(dbgs() << "The checksums for " << IRFunc.getName()
                        << "(IR) and " << ProfFunc << "(Profile) match.\n");

      return true;
    }
  }

  AnchorMap IRAnchors;
  findIRAnchors(IRFunc, IRAnchors);
  AnchorMap ProfileAnchors;
  findProfileAnchors(*FSFlattened, ProfileAnchors);

  AnchorList FilteredIRAnchorsList;
  AnchorList FilteredProfileAnchorList;
  getFilteredAnchorList(IRAnchors, ProfileAnchors, FilteredIRAnchorsList,
                        FilteredProfileAnchorList);

  // Similarly skip the matching if the num of anchors is not enough.
  if (FilteredIRAnchorsList.size() < MinCallCountForCGMatching ||
      FilteredProfileAnchorList.size() < MinCallCountForCGMatching)
    return false;

  // Use the diff algorithm to find the LCS between IR and profile.
  LocToLocMap MatchedAnchors =
      longestCommonSequence(FilteredIRAnchorsList, FilteredProfileAnchorList);

  Similarity =
      static_cast<float>(MatchedAnchors.size()) * 2 /
      (FilteredIRAnchorsList.size() + FilteredProfileAnchorList.size());

  LLVM_DEBUG(dbgs() << "The similarity between " << IRFunc.getName()
                    << "(IR) and " << ProfFunc << "(profile) is "
                    << format("%.2f", Similarity) << "\n");
  assert((Similarity >= 0 && Similarity <= 1.0) &&
         "Similarity value should be in [0, 1]");
  return Similarity * 100 > FuncProfileSimilarityThreshold;
}

bool SampleProfileMatcher::functionMatchesProfile(Function &IRFunc,
                                                  const FunctionId &ProfFunc) {
  auto R = FunctionProfileNameMap.find({&IRFunc, ProfFunc});
  if (R != FunctionProfileNameMap.end())
    return R->second;

  bool Matched = functionMatchesProfileHelper(IRFunc, ProfFunc);
  FunctionProfileNameMap[{&IRFunc, ProfFunc}] = Matched;
  ProfileNameToFuncMap[ProfFunc] = &IRFunc;
  return Matched;
}

void SampleProfileMatcher::matchCalleeProfile(
    const FunctionId &Caller, const FunctionId &ProfileCalleeName,
    const std::vector<Function *> *IRCalleesToMatch,
    std::vector<std::pair<FunctionId, FunctionId>> &MatchResult) {
  if (!IRCalleesToMatch)
    return;
  // Check whether this is an existing function matching the profile, we only
  // run the matching when the callee profile is unused.
  auto F = SymbolMap->find(ProfileCalleeName);
  if (F != SymbolMap->end())
    return;

  for (auto *IRFuncCandidate : *IRCalleesToMatch)
    if (functionMatchesProfile(*IRFuncCandidate, ProfileCalleeName)) {
      FunctionId IRCalleeName(IRFuncCandidate->getName());
      assert(IRCalleeName != ProfileCalleeName &&
             "New callee symbol is not a new function");
      LLVM_DEBUG(dbgs() << "In function " << Caller
                        << ", changing profile name from " << ProfileCalleeName
                        << " to " << IRCalleeName << "\n");
      MatchResult.emplace_back(IRCalleeName, ProfileCalleeName);
      return;
    }
}

// Traverse the profiled call-graph recursively to run the matching.
void SampleProfileMatcher::matchProfileForNewFunctions(
    const StringMap<Function *> &NewIRFunctions, FunctionSamples &FuncProfile) {
  // Find the new candidate IR callees in the current caller scope.
  std::vector<Function *> *IRCalleesToMatch = nullptr;
  if (auto *IRCaller = findFuncByProfileName(FuncProfile.getFunction())) {
    // No callees for external function, skip the rename matching.
    if (IRCaller->isDeclaration())
      return;
    IRCalleesToMatch = findNewIRCallees(*IRCaller, NewIRFunctions);
  }
  // Don't return here when IRCalleesToMatch is nullptr or empty, this is
  // because even if there is no matching in the current scope, there could be
  // matching in deeper callee scope/edge, so we need to keep traversing the
  // call-graph. For IRCalleesToMatch is nullptr or empty case, later the
  // matching function(matchCalleeProfile) will handle this to make it non-op.

  // Match non-inline callees.
  for (auto &BS : const_cast<BodySampleMap &>(FuncProfile.getBodySamples())) {
    // New function to old function pairs used to update the CallTargetMap.
    std::vector<std::pair<FunctionId, FunctionId>> MatchResult;
    SampleRecord::CallTargetMap &CTM =
        const_cast<SampleRecord::CallTargetMap &>(BS.second.getCallTargets());
    for (const auto &TS : CTM)
      matchCalleeProfile(FuncProfile.getFunction(), TS.first, IRCalleesToMatch,
                         MatchResult);

    // Update the CallTargetMap.
    for (const auto &P : MatchResult) {
      CTM[P.first] = CTM[P.second];
      CTM.erase(P.second);
    }
  }

  // Match inline callees.
  for (auto &CM :
       const_cast<CallsiteSampleMap &>(FuncProfile.getCallsiteSamples())) {
    auto &CalleeMap = CM.second;
    // New function to old function pairs used to update the CallsiteSampleMap.
    std::vector<std::pair<FunctionId, FunctionId>> MatchResult;
    for (auto &CS : CalleeMap) {
      FunctionSamples &CalleeProfile = CS.second;
      matchCalleeProfile(FuncProfile.getFunction(), CalleeProfile.getFunction(),
                         IRCalleesToMatch, MatchResult);

      // Traverse all the inlined callee profiles.
      matchProfileForNewFunctions(NewIRFunctions, CalleeProfile);
    }

    // Update the CalleeMap using the new name and remove the old entry.
    for (auto &P : MatchResult) {
      assert(P.first != P.second &&
             "Renamed function name should be different from the old map key");
      FunctionSamples &FS = CalleeMap[P.second];
      FS.setFunction(P.first);
      CalleeMap[P.first] = FS;
      CalleeMap.erase(P.second);
    }
  }
}

// Match the unused profile with new IR functions on the profiled call-graph.
// The high-level steps for the algorithm:
// 1) Find all the new functions that show only in the IR, use them as the
// matching candidates to compute new callees.
//
// 2) Traverse all the nodes in the profiled call-graph.
// For each function caller scope:
//  a) Find a set of callees in the IR that doesn't exist in the profile. See
//  findNewIRCallees.
//  b) Find a set of callees in the profile that doesn't exist
//  in the IR. See matchCalleeProfile.
//  c) Match the callee pairs between a and b. Compute a similarity ratio
//  between the pair, it's considered match if the similarity is above a given
//  threshold. See MatchProfileForNewFunctions.
//  d) Update the profile with the matched name in-place.
void SampleProfileMatcher::runCallGraphMatching() {
  if (!SalvageUnusedProfile)
    return;
  assert(SymbolMap && "SymbolMap is null");
  assert(FunctionProfileNameMap.empty() &&
         "FunctionProfileNameMap is not empty before the call graph matching");

  StringMap<Function *> NewIRFunctions;
  findNewIRFunctions(NewIRFunctions);
  if (NewIRFunctions.empty())
    return;

  // Sort the profiles to make the matching order deterministic.
  std::vector<NameFunctionSamples> SortedProfiles;
  ::llvm::sortFuncProfiles(Reader.getProfiles(), SortedProfiles);
  for (auto &P : SortedProfiles)
    matchProfileForNewFunctions(NewIRFunctions,
                                *const_cast<FunctionSamples *>(P.second));

  clearCacheData();
  if (ProfileNameToFuncMap.empty())
    return;
  // Update all the data generated by the old profile.
  // Add the new function to the SymbolMap, which will be used in
  // SampleLoader.
  for (auto &I : ProfileNameToFuncMap) {
    assert(I.second && "New function is null");
    SymbolMap->emplace(FunctionId(I.second->getName()), I.second);
  }

  // Re-flatten the profiles after the matching.
  FlattenedProfiles.clear();
  ProfileConverter::flattenProfile(Reader.getProfiles(), FlattenedProfiles,
                                   FunctionSamples::ProfileIsCS);
  ProfileNameToFuncMap.clear();
}

void SampleProfileMatcher::runOnFunction(Function &F) {
  if (skipProfileForFunction(F))
    return;
  runCFGMatching(F);
}

void SampleProfileMatcher::runOnModule(
    HashKeyMap<std::unordered_map, FunctionId, Function *> &SymMap) {
  ProfileConverter::flattenProfile(Reader.getProfiles(), FlattenedProfiles,
                                   FunctionSamples::ProfileIsCS);
  SymbolMap = &SymMap;
  runCallGraphMatching();

  for (auto &F : M)
    runOnFunction(F);

  if (SalvageStaleProfile)
    distributeIRToProfileLocationMap();

  computeAndReportProfileStaleness();
}

void SampleProfileMatcher::distributeIRToProfileLocationMap(
    FunctionSamples &FS) {
  const auto ProfileMappings = FuncMappings.find(FS.getFuncName());
  if (ProfileMappings != FuncMappings.end()) {
    FS.setIRToProfileLocationMap(&(ProfileMappings->second));
  }

  for (auto &Callees :
       const_cast<CallsiteSampleMap &>(FS.getCallsiteSamples())) {
    for (auto &FS : Callees.second) {
      distributeIRToProfileLocationMap(FS.second);
    }
  }
}

// Use a central place to distribute the matching results. Outlined and inlined
// profile with the function name will be set to the same pointer.
void SampleProfileMatcher::distributeIRToProfileLocationMap() {
  for (auto &I : Reader.getProfiles()) {
    distributeIRToProfileLocationMap(I.second);
  }
}
