//===------ MacroFusionPredicatorEmitter.cpp - Generator for Fusion ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===---------------------------------------------------------------------===//
//
// MacroFusionPredicatorEmitter implements a TableGen-driven predicators
// generator for macro-op fusions.
//
//===---------------------------------------------------------------------===//

#include "CodeGenTarget.h"
#include "PredicateExpander.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Debug.h"
#include "llvm/TableGen/Error.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"
#include <set>
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "macro-fusion-predicator"

namespace {
class MacroFusionPredicatorEmitter {
  RecordKeeper &Records;
  CodeGenTarget Target;

  void emitMacroFusionDecl(std::vector<Record *> Fusions, PredicateExpander &PE,
                           raw_ostream &OS);
  void emitMacroFusionImpl(std::vector<Record *> Fusions, PredicateExpander &PE,
                           raw_ostream &OS);
  void emitPredicates(std::vector<Record *> &FirstPredicate,
                      PredicateExpander &PE, raw_ostream &OS);
  void emitFirstPredicate(Record *SecondPredicate, PredicateExpander &PE,
                          raw_ostream &OS);
  void emitSecondPredicate(Record *SecondPredicate, PredicateExpander &PE,
                           raw_ostream &OS);
  void emitBothPredicate(Record *Predicates, PredicateExpander &PE,
                         raw_ostream &OS);

public:
  MacroFusionPredicatorEmitter(RecordKeeper &R) : Records(R), Target(R) {}

  void run(raw_ostream &OS);
};
} // End anonymous namespace.

void MacroFusionPredicatorEmitter::emitMacroFusionDecl(
    std::vector<Record *> Fusions, PredicateExpander &PE, raw_ostream &OS) {
  OS << "#ifdef GET_" << Target.getName() << "_MACRO_FUSION_PRED_DECL\n\n";

  for (Record *Fusion : Fusions) {
    OS << "bool is" << Fusion->getName() << "(const TargetInstrInfo &, "
       << "const TargetSubtargetInfo &, "
       << "const MachineInstr *, "
       << "const MachineInstr &);\n";
  }

  OS << "\n#endif\n";
  OS << "#undef GET_" << Target.getName() << "_MACRO_FUSION_PRED_DECL\n";
}

void MacroFusionPredicatorEmitter::emitMacroFusionImpl(
    std::vector<Record *> Fusions, PredicateExpander &PE, raw_ostream &OS) {
  OS << "#ifdef GET_" << Target.getName() << "_MACRO_FUSION_PRED_IMPL\n\n";

  for (Record *Fusion : Fusions) {
    std::vector<Record *> Predicates =
        Fusion->getValueAsListOfDefs("Predicates");

    OS << "bool is" << Fusion->getName() << "(\n";
    OS.indent(5) << "const TargetInstrInfo &TII,\n";
    OS.indent(5) << "const TargetSubtargetInfo &STI,\n";
    OS.indent(5) << "const MachineInstr *FirstMI,\n";
    OS.indent(5) << "const MachineInstr &SecondMI) {\n";
    OS.indent(2) << "auto &MRI = SecondMI.getMF()->getRegInfo();\n";

    emitPredicates(Predicates, PE, OS);

    OS.indent(2) << "return true;\n";
    OS << "}\n";
  }

  OS << "\n#endif\n";
  OS << "#undef GET_" << Target.getName() << "_MACRO_FUSION_PRED_IMPL\n\n";
}

void MacroFusionPredicatorEmitter::emitPredicates(
    std::vector<Record *> &Predicates, PredicateExpander &PE, raw_ostream &OS) {
  for (Record *Predicate : Predicates) {
    Record *Target = Predicate->getValueAsDef("Target");
    if (Target->getName() == "first")
      emitFirstPredicate(Predicate, PE, OS);
    else if (Target->getName() == "second")
      emitSecondPredicate(Predicate, PE, OS);
    else if (Target->getName() == "both")
      emitBothPredicate(Predicate, PE, OS);
    else
      PrintFatalError(Target->getLoc(),
                      "Unsupported 'FusionTarget': " + Target->getName());
  }
}

void MacroFusionPredicatorEmitter::emitFirstPredicate(Record *Predicate,
                                                      PredicateExpander &PE,
                                                      raw_ostream &OS) {
  if (Predicate->isSubClassOf("WildcardPred")) {
    OS.indent(2) << "if (!FirstMI)\n";
    OS.indent(2) << "  return "
                 << (Predicate->getValueAsBit("ReturnValue") ? "true" : "false")
                 << ";\n";
  } else if (Predicate->isSubClassOf("OneUsePred")) {
    OS.indent(2) << "{\n";
    OS.indent(4) << "Register FirstDest = FirstMI->getOperand(0).getReg();\n";
    OS.indent(4)
        << "if (FirstDest.isVirtual() && !MRI.hasOneNonDBGUse(FirstDest))\n";
    OS.indent(4) << "  return false;\n";
    OS.indent(2) << "}\n";
  } else if (Predicate->isSubClassOf(
                 "FirstFusionPredicateWithMCInstPredicate")) {
    OS.indent(2) << "{\n";
    OS.indent(4) << "const MachineInstr *MI = FirstMI;\n";
    OS.indent(4) << "if (";
    PE.setNegatePredicate(true);
    PE.setIndentLevel(3);
    PE.expandPredicate(OS, Predicate->getValueAsDef("Predicate"));
    OS << ")\n";
    OS.indent(4) << "  return false;\n";
    OS.indent(2) << "}\n";
  } else {
    PrintFatalError(Predicate->getLoc(),
                    "Unsupported predicate for first instruction: " +
                        Predicate->getType()->getAsString());
  }
}

void MacroFusionPredicatorEmitter::emitSecondPredicate(Record *Predicate,
                                                       PredicateExpander &PE,
                                                       raw_ostream &OS) {
  if (Predicate->isSubClassOf("SecondFusionPredicateWithMCInstPredicate")) {
    OS.indent(2) << "{\n";
    OS.indent(4) << "const MachineInstr *MI = &SecondMI;\n";
    OS.indent(4) << "if (";
    PE.setNegatePredicate(true);
    PE.setIndentLevel(3);
    PE.expandPredicate(OS, Predicate->getValueAsDef("Predicate"));
    OS << ")\n";
    OS.indent(4) << "  return false;\n";
    OS.indent(2) << "}\n";
  } else {
    PrintFatalError(Predicate->getLoc(),
                    "Unsupported predicate for first instruction: " +
                        Predicate->getType()->getAsString());
  }
}

void MacroFusionPredicatorEmitter::emitBothPredicate(Record *Predicate,
                                                     PredicateExpander &PE,
                                                     raw_ostream &OS) {
  if (Predicate->isSubClassOf("FusionPredicateWithCode"))
    OS << Predicate->getValueAsString("Predicate");
  else if (Predicate->isSubClassOf("BothFusionPredicateWithMCInstPredicate")) {
    Record *MCPred = Predicate->getValueAsDef("Predicate");
    emitFirstPredicate(MCPred, PE, OS);
    emitSecondPredicate(MCPred, PE, OS);
  } else if (Predicate->isSubClassOf("TieReg")) {
    int FirstOpIdx = Predicate->getValueAsInt("FirstOpIdx");
    int SecondOpIdx = Predicate->getValueAsInt("SecondOpIdx");
    OS.indent(2) << "if (!(FirstMI->getOperand(" << FirstOpIdx
                 << ").isReg() &&\n";
    OS.indent(2) << "      SecondMI.getOperand(" << SecondOpIdx
                 << ").isReg() &&\n";
    OS.indent(2) << "      FirstMI->getOperand(" << FirstOpIdx
                 << ").getReg() == SecondMI.getOperand(" << SecondOpIdx
                 << ").getReg()))\n";
    OS.indent(2) << "  return false;\n";
  } else
    PrintFatalError(Predicate->getLoc(),
                    "Unsupported predicate for both instruction: " +
                        Predicate->getType()->getAsString());
}

void MacroFusionPredicatorEmitter::run(raw_ostream &OS) {
  // Emit file header.
  emitSourceFileHeader("Macro Fusion Predicators", OS);

  PredicateExpander PE(Target.getName());
  PE.setByRef(false);
  PE.setExpandForMC(false);

  std::vector<Record *> Fusions = Records.getAllDerivedDefinitions("Fusion");
  // Sort macro fusions by name.
  sort(Fusions, LessRecord());
  emitMacroFusionDecl(Fusions, PE, OS);
  emitMacroFusionImpl(Fusions, PE, OS);
}

static TableGen::Emitter::OptClass<MacroFusionPredicatorEmitter>
    X("gen-macro-fusion-pred", "Generate macro fusion predicators.");
