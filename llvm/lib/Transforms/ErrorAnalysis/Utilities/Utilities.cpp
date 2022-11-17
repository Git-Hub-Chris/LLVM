#include <string>
#include "llvm/Transforms/ErrorAnalysis/Utilities/Utilities.h"
#include "llvm/Transforms/ErrorAnalysis/AtomicCondition/AmplificationFactor.h"
#include "llvm/Transforms/ErrorAnalysis/Utilities/FunctionMatchers.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DebugInfo.h"

using namespace llvm;
using namespace atomiccondition;
using namespace std;

namespace atomiccondition {

// Get Instruction after any Phi/Dbg instructions AND Atomic Condition and
// Computation Graph Initialization Calls.
Instruction *getInstructionAfterInitializationCalls(BasicBlock *BB) {
  BasicBlock::iterator NextInst(BB->getFirstNonPHIOrDbg());

  // Iterating through instructions till we skip any Initialization calls.
  while(NextInst->getOpcode() == 56 &&
         (!static_cast<CallInst*>(&*NextInst)->getCalledFunction() ||
          (static_cast<CallInst*>(&*NextInst)->getCalledFunction() &&
           (static_cast<CallInst*>(&*NextInst)->getCalledFunction()->getName().str() == "fACCreate" ||
            static_cast<CallInst*>(&*NextInst)->getCalledFunction()->getName().str() == "fCGInitialize"))))
    NextInst++;

  return &(*NextInst);
}

Value *createBBNameGlobalString(BasicBlock *BB) {
  string BasicBlockString;
  raw_string_ostream RawBasicBlockString(BasicBlockString);
  BB->printAsOperand(RawBasicBlockString, false);
  Constant *BBValue = ConstantDataArray::getString(BB->getModule()->getContext(),
                                                   RawBasicBlockString.str().c_str(),
                                                   true);
  
  Value *BBValuePointer = new GlobalVariable(*BB->getModule(),
                                             BBValue->getType(),
                                             true,
                                             GlobalValue::InternalLinkage,
                                             BBValue);

  return BBValuePointer;
}

Value *createRegisterNameGlobalString(Instruction *Inst) {
  string RegisterString;
  raw_string_ostream RawRegisterString(RegisterString);
  Inst->printAsOperand(RawRegisterString, false);

  Constant *RegisterValue = ConstantDataArray::getString(Inst->getModule()->getContext(),
                                                         RawRegisterString.str().c_str(),
                                                         true);

  Value *RegisterValuePointer = new GlobalVariable(*Inst->getModule(),
                                                   RegisterValue->getType(),
                                                   true,
                                                   GlobalValue::InternalLinkage,
                                                   RegisterValue);

  return RegisterValuePointer;
}

Value *createInstructionGlobalString(Instruction *Inst) {
  string InstructionString;
  raw_string_ostream RawInstructionString(InstructionString);
  RawInstructionString << *Inst;
  unsigned long NonEmptyPosition= RawInstructionString.str().find_first_not_of(" \n\r\t\f\v");
  string Initializer = (NonEmptyPosition == std::string::npos) ? "" :
                                                               RawInstructionString.str().substr(NonEmptyPosition);

  Constant *InstructionValue = ConstantDataArray::getString(Inst->getModule()->getContext(),
                                                            Initializer.c_str(),
                                                            true);

  Value *InstructionValuePointer = new GlobalVariable(*Inst->getModule(),
                                                      InstructionValue->getType(),
                                                      true,
                                                      GlobalValue::InternalLinkage,
                                                      InstructionValue);

  return InstructionValuePointer;
}

Value *createStringRefGlobalString(StringRef StringObj, Instruction *Inst) {
  string StringRefString;
  raw_string_ostream RawInstructionString(StringRefString);
  RawInstructionString << StringObj;
  unsigned long NonEmptyPosition= RawInstructionString.str().find_first_not_of(" \n\r\t\f\v");
  string Initializer = (NonEmptyPosition == std::string::npos) ? "" :
                                                               RawInstructionString.str().substr(NonEmptyPosition);

  Constant *StringObjValue = ConstantDataArray::getString(Inst->getModule()->getContext(),
                                                          Initializer.c_str(),
                                                          true);

  Value *StringObjValuePointer = new GlobalVariable(*Inst->getModule(),
                                                    StringObjValue->getType(),
                                                    true,
                                                    GlobalValue::InternalLinkage,
                                                    StringObjValue);

  return StringObjValuePointer;
}

string getInstructionAsString(Instruction *Inst) {
  string InstructionString;
  raw_string_ostream RawInstructionString(InstructionString);
  RawInstructionString << *Inst;
  unsigned long NonEmptyPosition= RawInstructionString.str().find_first_not_of(" \n\r\t\f\v");
  string InstructionAsString = (NonEmptyPosition == std::string::npos) ? "" :
                                                                       RawInstructionString.str().substr(NonEmptyPosition);
  return InstructionAsString;
}

bool isInstructionOfInterest(Instruction *Inst) {
  switch (Inst->getOpcode()) {
  case 14:
  case 16:
  case 18:
  case 21:
  case 31:
  case 32:
  case 55:
  case 56:
  case 57:
    return true;
  }
  return false;
}

int getFunctionEnum(Instruction *Inst) {
  string FunctionName = "";

  switch (Inst->getOpcode()) {
  case 12:
    return Func::Neg;
  case 14:
    return Func::Add;
  case 16:
    return Func::Sub;
  case 18:
    return Func::Mul;
  case 21:
    return Func::Div;
  case 56:
    if(static_cast<CallInst*>(Inst)->getCalledFunction() &&
        static_cast<CallInst*>(Inst)->getCalledFunction()->hasName())
      FunctionName = static_cast<CallInst*>(Inst)->getCalledFunction()->getName().str();
    transform(FunctionName.begin(), FunctionName.end(), FunctionName.begin(), ::tolower);
    if (isASinFunction(FunctionName))
      return Func::ArcSin;
    if(isACosFunction(FunctionName))
      return Func::ArcCos;
    if(isATanFunction(FunctionName))
      return Func::ArcTan;
    if(isSinhFunction(FunctionName))
      return Func::Sinh;
    if(isCoshFunction(FunctionName))
      return Func::Cosh;
    if(isTanhFunction(FunctionName))
      return Func::Tanh;
    if(isSinFunction(FunctionName))
      return Func::Sin;
    if(isCosFunction(FunctionName))
      return Func::Cos;
    if(isTanFunction(FunctionName))
      return Func::Tan;
    if(isExpFunction(FunctionName))
      return Func::Exp;
    if(isLogFunction(FunctionName))
      return Func::Log;
    if(isSqrtFunction(FunctionName))
      return Func::Sqrt;
    if(isFMAFuncton(FunctionName))
      return Func::FMA;
    return -1;
  default:
    //    errs() << BaseInstruction << " is not a Binary Instruction.\n";
    return -1;
  }
}

} // namespace atomiccondition