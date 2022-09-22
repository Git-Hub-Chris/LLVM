//
// Created by tanmay on 6/10/22.
//

#include <string>
#include "llvm/Transforms/ErrorAnalysis/AtomicCondition/ACInstrumentation.h"
#include "llvm/Transforms/ErrorAnalysis/AtomicCondition/ComputationGraph.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/DebugInfo.h"

using namespace llvm;
using namespace atomiccondition;
using namespace std;

void confFunction(Function *FunctionToSave, Function **StorageLocation,
                  GlobalValue::LinkageTypes LinkageType)
{
  // Save the function pointer
  if(StorageLocation != nullptr)
    *StorageLocation = FunctionToSave;
  if (FunctionToSave->getLinkage() != LinkageType)
    FunctionToSave->setLinkage(LinkageType);
}

#define SET_ODR_LIKAGE(name) \
    if (CurrentFunction->getName().str().find(name) != std::string::npos) { \
      CurrentFunction->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage); \
    }

// Searches module for functions to mark and creates pointers for them.
ACInstrumentation::ACInstrumentation(Function *InstrumentFunction) : FunctionToInstrument(InstrumentFunction),
                                                                     ACInitFunction(nullptr),
                                                                     CGInitFunction(nullptr),
                                                                     AFInitFunction(nullptr),
                                                                     ACfp32UnaryFunction(nullptr),
                                                                     ACfp64UnaryFunction(nullptr),
                                                                     ACfp32BinaryFunction(nullptr),
                                                                     ACfp64BinaryFunction(nullptr),
                                                                     CGRecordPHIInstruction(nullptr),
                                                                     CGRecordBasicBlock(nullptr),
                                                                     CGCreateNode(nullptr),
                                                                     ACStoreFunction(nullptr),
                                                                     CGStoreFunction(nullptr),
                                                                     AFStoreFunction(nullptr),
                                                                     AFPrintTopAmplificationPaths(nullptr),
                                                                     AFfp32AnalysisFunction(nullptr),
                                                                     AFfp64AnalysisFunction(nullptr),
                                                                     CGDotGraphFunction(nullptr)
{
  // Find and configure instrumentation functions
  Module *M = FunctionToInstrument->getParent();

  // Configuring all runtime functions and saving pointers.
  for(Module::iterator F = M->begin(); F != M->end(); ++F) {
    Function *CurrentFunction = &*F;

    // Only configuring functions with certain prefixes
    if(!CurrentFunction->hasName()) {

    }
    else if (CurrentFunction->getName().str().find("fACCreate") != std::string::npos) {
      confFunction(CurrentFunction, &ACInitFunction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fCGInitialize") != std::string::npos) {
      confFunction(CurrentFunction, &CGInitFunction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fAFInitialize") != std::string::npos) {
      confFunction(CurrentFunction, &AFInitFunction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fACfp32UnaryDriver") != std::string::npos) {
      confFunction(CurrentFunction, &ACfp32UnaryFunction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fACfp64UnaryDriver") != std::string::npos) {
      confFunction(CurrentFunction, &ACfp64UnaryFunction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fACfp32BinaryDriver") != std::string::npos) {
      confFunction(CurrentFunction, &ACfp32BinaryFunction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fACfp64BinaryDriver") != std::string::npos) {
      confFunction(CurrentFunction, &ACfp64BinaryFunction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fCGInitialize") != std::string::npos) {
      confFunction(CurrentFunction, &CGInitFunction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fCGrecordPHIInstruction") != std::string::npos) {
      confFunction(CurrentFunction, &CGRecordPHIInstruction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fCGrecordCurrentBasicBlock") != std::string::npos) {
      confFunction(CurrentFunction, &CGRecordBasicBlock,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fCGcreateNode") != std::string::npos) {
      confFunction(CurrentFunction, &CGCreateNode,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fACStoreACs") != std::string::npos) {
      confFunction(CurrentFunction, &ACStoreFunction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fCGStoreCG") != std::string::npos) {
      confFunction(CurrentFunction, &CGStoreFunction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fAFStoreAFs") != std::string::npos) {
      confFunction(CurrentFunction, &AFStoreFunction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fAFPrintTopAmplificationPaths") != std::string::npos) {
      confFunction(CurrentFunction, &AFPrintTopAmplificationPaths,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fAFfp32Analysis") != std::string::npos) {
      confFunction(CurrentFunction, &AFfp32AnalysisFunction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fAFfp64Analysis") != std::string::npos) {
      confFunction(CurrentFunction, &AFfp64AnalysisFunction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
    else if (CurrentFunction->getName().str().find("fCGDotGraph") != std::string::npos) {
      confFunction(CurrentFunction, &CGDotGraphFunction,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    }
  }
}

void ACInstrumentation::instrumentCallRecordingBasicBlock(BasicBlock* CurrentBB,
                                                          long *NumInstrumentedInstructions) {
  BasicBlock::iterator NextInst(CurrentBB->getFirstNonPHIOrDbg());
  while(NextInst->getOpcode() == 56 &&
         (!static_cast<CallInst*>(&*NextInst)->getCalledFunction() ||
          (static_cast<CallInst*>(&*NextInst)->getCalledFunction() &&
           (static_cast<CallInst*>(&*NextInst)->getCalledFunction()->getName().str() == "fCGInitialize" ||
            static_cast<CallInst*>(&*NextInst)->getCalledFunction()->getName().str() == "fACCreate"))))
    NextInst++;
  IRBuilder<> InstructionBuilder( &(*NextInst) );
  std::vector<Value *> Args;

  CallInst *BBRecordingCallInstruction = nullptr;

  Args.push_back(createBBNameGlobalString(CurrentBB));

  BBRecordingCallInstruction = InstructionBuilder.CreateCall(CGRecordBasicBlock, Args);

  *NumInstrumentedInstructions+=1;
  assert(BBRecordingCallInstruction && "Invalid call instruction!");
}

void ACInstrumentation::instrumentCallRecordingPHIInstructions(BasicBlock* CurrentBB,
                                                               long *NumInstrumentedInstructions) {
  BasicBlock::iterator NextInst(CurrentBB->getFirstNonPHIOrDbg());
  while(NextInst->getOpcode() == 56 &&
         (!static_cast<CallInst*>(&*NextInst)->getCalledFunction() ||
          (static_cast<CallInst*>(&*NextInst)->getCalledFunction() &&
           (static_cast<CallInst*>(&*NextInst)->getCalledFunction()->getName().str() == "fCGInitialize" ||
            static_cast<CallInst*>(&*NextInst)->getCalledFunction()->getName().str() == "fACCreate"))))
      NextInst++;
  IRBuilder<> InstructionBuilder( &(*NextInst) );

  long int NumberOfCalls = 0;

  for (BasicBlock::phi_iterator_impl<> CurrPhi = CurrentBB->phis().begin();
       CurrPhi != CurrentBB->phis().end();
       CurrPhi++) {
    std::vector<Value *> Args;
    Args.push_back(createInstructionGlobalString(&*CurrPhi));
    Args.push_back(createBBNameGlobalString(CurrentBB));
    ArrayRef<Value *> ArgsRef(Args);

    InstructionBuilder.CreateCall(CGRecordPHIInstruction, ArgsRef);

    NumberOfCalls+=1;
  }

  *NumInstrumentedInstructions+=NumberOfCalls;
}

// Creates a node for LLVM memory load instructions in the computation graph.
void ACInstrumentation::instrumentCallsForMemoryLoadOperation(
    Instruction *BaseInstruction, long *NumInstrumentedInstructions) {
  assert((CGCreateNode!=nullptr) && "Function not initialized!");

  BasicBlock::iterator NextInst(BaseInstruction);
  NextInst++;
  IRBuilder<> InstructionBuilder( &(*NextInst) );
  std::vector<Value *> Args;

  CallInst *CGCallInstruction = nullptr;

  Constant *EmptyValue = ConstantDataArray::getString(BaseInstruction->getModule()->getContext(),
                                                            "",
                                                            true);

  Value *EmptyValuePointer = new GlobalVariable(*BaseInstruction->getModule(),
                                                      EmptyValue->getType(),
                                                      true,
                                                      GlobalValue::InternalLinkage,
                                                      EmptyValue);

  Value *FileNameValuePointer;
  int LineNumber;
  if(BaseInstruction->getDebugLoc()) {
    FileNameValuePointer = createStringRefGlobalString(
        BaseInstruction->getDebugLoc()->getDirectory(), BaseInstruction);
    LineNumber = BaseInstruction->getDebugLoc().getLine();
  }
  else {
    FileNameValuePointer = EmptyValuePointer;
    LineNumber = -1;
  }

  Args.push_back(createInstructionGlobalString(BaseInstruction));
  Args.push_back(EmptyValuePointer);
  Args.push_back(EmptyValuePointer);
  Args.push_back(InstructionBuilder.getInt32(NodeKind::Register));
  Args.push_back(FileNameValuePointer);
  Args.push_back(InstructionBuilder.getInt32(LineNumber));
  ArrayRef<Value *> ArgsRef(Args);

  CGCallInstruction = InstructionBuilder.CreateCall(CGCreateNode, ArgsRef);
  *NumInstrumentedInstructions+=1;

  assert(CGCallInstruction && "Invalid call instruction!");
  return;
}

// Creates a node for LLVM Cast instructions that DO NOT convert from one
// floating-point to another floating-point format in the computation graph.
void ACInstrumentation::instrumentCallsForCastOperation(
    Instruction *BaseInstruction, long *NumInstrumentedInstructions) {
  assert((CGCreateNode!=nullptr) && "Function not initialized!");

  BasicBlock::iterator NextInst(BaseInstruction);
  NextInst++;
  IRBuilder<> InstructionBuilder( &(*NextInst) );
  std::vector<Value *> Args;

  CallInst *CGCallInstruction = nullptr;

  Constant *EmptyValue = ConstantDataArray::getString(BaseInstruction->getModule()->getContext(),
                                                      "",
                                                      true);

  Value *EmptyValuePointer = new GlobalVariable(*BaseInstruction->getModule(),
                                                EmptyValue->getType(),
                                                true,
                                                GlobalValue::InternalLinkage,
                                                EmptyValue);

  Value *FileNameValuePointer;
  int LineNumber;
  if(BaseInstruction->getDebugLoc()) {
    FileNameValuePointer = createStringRefGlobalString(
        BaseInstruction->getDebugLoc()->getDirectory(), BaseInstruction);
    LineNumber = BaseInstruction->getDebugLoc().getLine();
  }
  else {
    FileNameValuePointer = EmptyValuePointer;
    LineNumber = -1;
  }

  Args.push_back(createInstructionGlobalString(BaseInstruction));
  Args.push_back(EmptyValuePointer);
  Args.push_back(EmptyValuePointer);
  Args.push_back(InstructionBuilder.getInt32(NodeKind::Register));
  Args.push_back(FileNameValuePointer);
  Args.push_back(InstructionBuilder.getInt32(LineNumber));
  ArrayRef<Value *> ArgsRef(Args);

  CGCallInstruction = InstructionBuilder.CreateCall(CGCreateNode, ArgsRef);
  *NumInstrumentedInstructions+=1;

  assert(CGCallInstruction && "Invalid call instruction!");
  return;
}

// Instruments a call to calculate atomic condition for unary floating point
// instructions and creates a node for this instruction in the computation graph.
void ACInstrumentation::instrumentCallsForUnaryOperation(Instruction* BaseInstruction,
                                            long *NumInstrumentedInstructions) {
  assert((CGCreateNode!=nullptr) && (ACfp32UnaryFunction!=nullptr) &&
         (ACfp64UnaryFunction!=nullptr) && "Function not initialized!");
  Operation OpType;
  string FunctionName = "";


  switch (BaseInstruction->getOpcode()) {
  case 12:
    OpType = Operation::Neg;
    break;
  case 45:
    assert(static_cast<FPTruncInst*>(BaseInstruction)->getDestTy()->isFloatTy());
    OpType = Operation::TruncToFloat;
    break;
  case 56:
    if(static_cast<CallInst*>(BaseInstruction)->getCalledFunction() &&
        static_cast<CallInst*>(BaseInstruction)->getCalledFunction()->hasName())
      FunctionName = static_cast<CallInst*>(BaseInstruction)->getCalledFunction()->getName().str();
    transform(FunctionName.begin(), FunctionName.end(), FunctionName.begin(), ::tolower);
    if (FunctionName.find("asin") != std::string::npos)
      OpType = Operation::ArcSin;
    else if(FunctionName.find("acos") != std::string::npos)
      OpType = Operation::ArcCos;
    else if(FunctionName.find("atan") != std::string::npos)
      OpType = Operation::ArcTan;
    else if(FunctionName.find("sinh") != std::string::npos)
      OpType = Operation::Sinh;
    else if(FunctionName.find("cosh") != std::string::npos)
      OpType = Operation::Cosh;
    else if(FunctionName.find("tanh") != std::string::npos)
      OpType = Operation::Tanh;
    else if(FunctionName.find("sin") != std::string::npos)
      OpType = Operation::Sin;
    else if(FunctionName.find("cos") != std::string::npos)
      OpType = Operation::Cos;
    else if(FunctionName.find("tan") != std::string::npos)
      OpType = Operation::Tan;
    else if(FunctionName.find("exp") != std::string::npos)
      OpType = Operation::Exp;
    else if(FunctionName.find("log") != std::string::npos)
      OpType = Operation::Log;
    else if(FunctionName.find("sqrt") != std::string::npos)
      OpType = Operation::Sqrt;
    else
      return;
    break;
  default:
    errs() << BaseInstruction << " is not a Binary Instruction.\n";
    return;
  }

  BasicBlock::iterator NextInst(BaseInstruction);
  NextInst++;
  IRBuilder<> InstructionBuilder( &(*NextInst) );

  //----------------------------------------------------------------------------
  //------------------ Instrumenting AC Calculating Function ------------------
  //----------------------------------------------------------------------------

  std::vector<Value *> ACArgs;

  Constant *EmptyValue = ConstantDataArray::getString(BaseInstruction->getModule()->getContext(),
                                                      "",
                                                      true);

  Value *EmptyValuePointer = new GlobalVariable(*BaseInstruction->getModule(),
                                                EmptyValue->getType(),
                                                true,
                                                GlobalValue::InternalLinkage,
                                                EmptyValue);

  Value *LeftOpRegisterNamePointer;
  if(!isa<Constant>(BaseInstruction->getOperand(0)) && !isa<Argument>(BaseInstruction->getOperand(0)))
    LeftOpRegisterNamePointer = createRegisterNameGlobalString(static_cast<Instruction*>(BaseInstruction->getOperand(0)));
  else
    LeftOpRegisterNamePointer = EmptyValuePointer;

  ACArgs.push_back(LeftOpRegisterNamePointer);
  ACArgs.push_back(BaseInstruction->getOperand(0));

  ACArgs.push_back(InstructionBuilder.getInt32(OpType));

  ArrayRef<Value *> ACArgsRef(ACArgs);

  CallInst *ACComputingCallInstruction = nullptr;

  // Branch based on data type of operation
  if(isSingleFPOperation(BaseInstruction)) {
    ACComputingCallInstruction =
        InstructionBuilder.CreateCall(ACfp32UnaryFunction, ACArgsRef);
    *NumInstrumentedInstructions+=1;
  }
  else if(isDoubleFPOperation(BaseInstruction)) {
    ACComputingCallInstruction =
        InstructionBuilder.CreateCall(ACfp64UnaryFunction, ACArgsRef);
    *NumInstrumentedInstructions+=1;
  }

  //----------------------------------------------------------------------------
  //----------------- Instrumenting CG Node creating function -----------------
  //----------------------------------------------------------------------------
  std::vector<Value *> CGArgs;

  CallInst *CGCallInstruction = nullptr;

  Value *LeftOpInstructionValuePointer;
  if(!isa<Constant>(BaseInstruction->getOperand(0)) && !isa<Argument>(BaseInstruction->getOperand(0)))
    LeftOpInstructionValuePointer = createInstructionGlobalString(static_cast<Instruction*>(BaseInstruction->getOperand(0)));
  else
    LeftOpInstructionValuePointer = EmptyValuePointer;

  Value *FileNameValuePointer;
  int LineNumber;
  if(BaseInstruction->getDebugLoc()) {
    FileNameValuePointer = createStringRefGlobalString(
        BaseInstruction->getDebugLoc()->getDirectory(), BaseInstruction);
    LineNumber = BaseInstruction->getDebugLoc().getLine();
  }
  else {
    FileNameValuePointer = EmptyValuePointer;
    LineNumber = -1;
  }

  CGArgs.push_back(createInstructionGlobalString(BaseInstruction));
  CGArgs.push_back(LeftOpInstructionValuePointer);
  CGArgs.push_back(EmptyValuePointer);
  CGArgs.push_back(InstructionBuilder.getInt32(NodeKind::UnaryInstruction));
  CGArgs.push_back(FileNameValuePointer);
  CGArgs.push_back(InstructionBuilder.getInt32(LineNumber));
  ArrayRef<Value *> CGArgsRef(CGArgs);

  CGCallInstruction = InstructionBuilder.CreateCall(CGCreateNode, CGArgsRef);
  *NumInstrumentedInstructions+=1;
  assert(ACComputingCallInstruction && CGCallInstruction && "Invalid call instruction!");
  return;
}

// Instruments a call to calculate atomic condition for binary floating point
// instructions and creates a node for this instruction in the computation graph.
void ACInstrumentation::instrumentCallsForBinaryOperation(Instruction* BaseInstruction,
                                             long *NumInstrumentedInstructions) {
  assert((CGCreateNode!=nullptr) && (ACfp32BinaryFunction!=nullptr) &&
         (ACfp64BinaryFunction!=nullptr) && "Function not initialized!");

  Operation OpType;
  switch (BaseInstruction->getOpcode()) {
  case 14:
    OpType = Operation::Add;
    break;
  case 16:
    OpType = Operation::Sub;
    break;
  case 18:
    OpType = Operation::Mul;
    break;
  case 21:
    OpType = Operation::Div;
    break;
  default:
    errs() << BaseInstruction << " is not a Binary Instruction.\n";
    return;
  }

  BasicBlock::iterator NextInst(BaseInstruction);
  NextInst++;
  IRBuilder<> InstructionBuilder( &(*NextInst) );

  //----------------------------------------------------------------------------
  //------------------ Instrumenting AC Calculating Function ------------------
  //----------------------------------------------------------------------------
//  if (DILocation *Loc = BaseInstruction->getDebugLoc()) // only true if dbg info exists
//  {
//    errs() << "Debug info:\n";
//    unsigned Line = Loc->getLine();
//    StringRef File = Loc->getFilename();
//    StringRef Dir = Loc->getDirectory();
//    errs() << Dir.str() << "/" << File.str() << ":" << to_string(Line);
////        NumberToString<unsigned>(Line);
//  }
//  else
//  {
//    errs() << "NONE";
//  }

  std::vector<Value *> Args;

  Constant *EmptyValue = ConstantDataArray::getString(BaseInstruction->getModule()->getContext(),
                                                      "",
                                                      true);

  Value *EmptyValuePointer = new GlobalVariable(*BaseInstruction->getModule(),
                                                EmptyValue->getType(),
                                                true,
                                                GlobalValue::InternalLinkage,
                                                EmptyValue);

  Value *LeftOpRegisterNamePointer;
  if(!isa<Constant>(BaseInstruction->getOperand(0)) && !isa<Argument>(BaseInstruction->getOperand(0)))
    LeftOpRegisterNamePointer = createRegisterNameGlobalString(static_cast<Instruction*>(BaseInstruction->getOperand(0)));
  else
    LeftOpRegisterNamePointer = EmptyValuePointer;

  Value *RightOpRegisterNamePointer;
  if(!isa<Constant>(BaseInstruction->getOperand(1)) && !isa<Argument>(BaseInstruction->getOperand(1)))
    RightOpRegisterNamePointer = createRegisterNameGlobalString(static_cast<Instruction*>(BaseInstruction->getOperand(1)));
  else
    RightOpRegisterNamePointer = EmptyValuePointer;

  Args.push_back(LeftOpRegisterNamePointer);
  Args.push_back(BaseInstruction->getOperand(0));
  Args.push_back(RightOpRegisterNamePointer);
  Args.push_back(BaseInstruction->getOperand(1));
  Args.push_back(InstructionBuilder.getInt32(OpType));

  ArrayRef<Value *> ArgsRef(Args);

  CallInst *ACComputingCallInstruction = nullptr;

  // Branch based on data type of operation
  if(isSingleFPOperation(BaseInstruction)) {
    ACComputingCallInstruction =
        InstructionBuilder.CreateCall(ACfp32BinaryFunction, ArgsRef);
    *NumInstrumentedInstructions+=1;
  }
  else if(isDoubleFPOperation(BaseInstruction)) {
    ACComputingCallInstruction =
        InstructionBuilder.CreateCall(ACfp64BinaryFunction, ArgsRef);
    *NumInstrumentedInstructions+=1;
  }

  //----------------------------------------------------------------------------
  //----------------- Instrumenting CG Node creating function -----------------
  //----------------------------------------------------------------------------
  std::vector<Value *> CGArgs;

  CallInst *CGCallInstruction = nullptr;

  Value *LeftOpInstructionValuePointer;
  if(!isa<Constant>(BaseInstruction->getOperand(0)) && !isa<Argument>(BaseInstruction->getOperand(0)))
    LeftOpInstructionValuePointer = createInstructionGlobalString(static_cast<Instruction*>(BaseInstruction->getOperand(0)));
  else
    LeftOpInstructionValuePointer = EmptyValuePointer;

  Value *RightOpInstructionValuePointer;
  if(!isa<Constant>(BaseInstruction->getOperand(1)) && !isa<Argument>(BaseInstruction->getOperand(1)))
    RightOpInstructionValuePointer = createInstructionGlobalString(static_cast<Instruction*>(BaseInstruction->getOperand(1)));
  else
    RightOpInstructionValuePointer = EmptyValuePointer;

  Value *FileNameValuePointer;
  int LineNumber;
  if(BaseInstruction->getDebugLoc()) {
    FileNameValuePointer = createStringRefGlobalString(
        BaseInstruction->getDebugLoc()->getDirectory(), BaseInstruction);
    LineNumber = BaseInstruction->getDebugLoc().getLine();
  }
  else {
    FileNameValuePointer = EmptyValuePointer;
    LineNumber = -1;
  }

  CGArgs.push_back(createInstructionGlobalString(BaseInstruction));
  CGArgs.push_back(LeftOpInstructionValuePointer);
  CGArgs.push_back(RightOpInstructionValuePointer);
  CGArgs.push_back(InstructionBuilder.getInt32(NodeKind::BinaryInstruction));
  CGArgs.push_back(FileNameValuePointer);
  CGArgs.push_back(InstructionBuilder.getInt32(LineNumber));
  ArrayRef<Value *> CGArgsRef(CGArgs);

  CGCallInstruction = InstructionBuilder.CreateCall(CGCreateNode, CGArgsRef);
  *NumInstrumentedInstructions+=1;

  assert(ACComputingCallInstruction && CGCallInstruction && "Invalid call instruction!");
  return;
}

void ACInstrumentation::instrumentCallsForOtherOperation(
    Instruction *BaseInstruction, long *NumInstrumentedInstructions) {
  assert((CGCreateNode!=nullptr) && (ACfp32BinaryFunction!=nullptr) &&
         (ACfp64BinaryFunction!=nullptr) && "Function not initialized!");
  BasicBlock::iterator NextInst(BaseInstruction);
  NextInst++;
  IRBuilder<> InstructionBuilder( &(*NextInst) );

  if (BaseInstruction->getOpcode() == Instruction::Call &&
    static_cast<const CallInst*>(BaseInstruction)->getCalledFunction() &&
    static_cast<const CallInst*>(BaseInstruction)->getCalledFunction()->hasName() &&
    (static_cast<const CallInst*>(BaseInstruction)->getCalledFunction()->getName().str().find("llvm.fmuladd") !=
         std::string::npos ||
    static_cast<const CallInst*>(BaseInstruction)->getCalledFunction()->getName().str().find("llvm.fma") !=
         std::string::npos)) {
    //----------------------------------------------------------------------------
    //------------------ Instrumenting AC Calculating Function ------------------
    //----------------------------------------------------------------------------

    std::vector<Value *> Args;

    Constant *EmptyValue = ConstantDataArray::getString(BaseInstruction->getModule()->getContext(),
                                                        "",
                                                        true);

    Value *EmptyValuePointer = new GlobalVariable(*BaseInstruction->getModule(),
                                                  EmptyValue->getType(),
                                                  true,
                                                  GlobalValue::InternalLinkage,
                                                  EmptyValue);

    Value *LeftOpRegisterNamePointer;
    if(!isa<Constant>(static_cast<const CallInst*>(BaseInstruction)->getArgOperand(1)) && !isa<Argument>(static_cast<const CallInst*>(BaseInstruction)->getArgOperand(1)))
      LeftOpRegisterNamePointer = createRegisterNameGlobalString(static_cast<Instruction*>(static_cast<const CallInst*>(BaseInstruction)->getArgOperand(1)));
    else
      LeftOpRegisterNamePointer = EmptyValuePointer;

    Value *RightOpRegisterNamePointer;
    if(!isa<Constant>(static_cast<const CallInst*>(BaseInstruction)->getArgOperand(2)) && !isa<Argument>(static_cast<const CallInst*>(BaseInstruction)->getArgOperand(2)))
      RightOpRegisterNamePointer = createRegisterNameGlobalString(static_cast<Instruction*>(static_cast<const CallInst*>(BaseInstruction)->getArgOperand(2)));
    else
      RightOpRegisterNamePointer = EmptyValuePointer;

    Args.push_back(LeftOpRegisterNamePointer);
    Args.push_back(static_cast<const CallInst*>(BaseInstruction)->getArgOperand(1));
    Args.push_back(RightOpRegisterNamePointer);
    Args.push_back(static_cast<const CallInst*>(BaseInstruction)->getArgOperand(2));
    Args.push_back(InstructionBuilder.getInt32(Operation::Add));

    ArrayRef<Value *> ArgsRef(Args);

    CallInst *ACComputingCallInstruction = nullptr;

    // Branch based on data type of operation
    if(isSingleFPOperation(BaseInstruction)) {
      ACComputingCallInstruction =
          InstructionBuilder.CreateCall(ACfp32BinaryFunction, ArgsRef);
      *NumInstrumentedInstructions+=1;
    }
    else if(isDoubleFPOperation(BaseInstruction)) {
      ACComputingCallInstruction =
          InstructionBuilder.CreateCall(ACfp64BinaryFunction, ArgsRef);
      *NumInstrumentedInstructions+=1;
    }

    //----------------------------------------------------------------------------
    //----------------- Instrumenting CG Node creating function -----------------
    //----------------------------------------------------------------------------
    std::vector<Value *> CGArgs;

    CallInst *CGCallInstruction = nullptr;

    Value *LeftOpInstructionValuePointer;
    if(!isa<Constant>(static_cast<const CallInst*>(BaseInstruction)->getArgOperand(1)) && !isa<Argument>(static_cast<const CallInst*>(BaseInstruction)->getArgOperand(1)))
      LeftOpInstructionValuePointer = createInstructionGlobalString(static_cast<Instruction*>(static_cast<const CallInst*>(BaseInstruction)->getArgOperand(1)));
    else
      LeftOpInstructionValuePointer = EmptyValuePointer;

    Value *RightOpInstructionValuePointer;
    if(!isa<Constant>(static_cast<const CallInst*>(BaseInstruction)->getArgOperand(2)) && !isa<Argument>(static_cast<const CallInst*>(BaseInstruction)->getArgOperand(2)))
      RightOpInstructionValuePointer = createInstructionGlobalString(static_cast<Instruction*>(static_cast<const CallInst*>(BaseInstruction)->getArgOperand(2)));
    else
      RightOpInstructionValuePointer = EmptyValuePointer;

    Value *FileNameValuePointer;
    int LineNumber;
    if(BaseInstruction->getDebugLoc()) {
      FileNameValuePointer = createStringRefGlobalString(
          BaseInstruction->getDebugLoc()->getDirectory(), BaseInstruction);
      LineNumber = BaseInstruction->getDebugLoc().getLine();
    }
    else {
      FileNameValuePointer = EmptyValuePointer;
      LineNumber = -1;
    }

    CGArgs.push_back(createInstructionGlobalString(BaseInstruction));
    CGArgs.push_back(LeftOpInstructionValuePointer);
    CGArgs.push_back(RightOpInstructionValuePointer);
    CGArgs.push_back(InstructionBuilder.getInt32(NodeKind::BinaryInstruction));
    CGArgs.push_back(FileNameValuePointer);
    CGArgs.push_back(InstructionBuilder.getInt32(LineNumber));
    ArrayRef<Value *> CGArgsRef(CGArgs);

    CGCallInstruction = InstructionBuilder.CreateCall(CGCreateNode, CGArgsRef);
    *NumInstrumentedInstructions+=1;

    assert(ACComputingCallInstruction && CGCallInstruction && "Invalid call instruction!");

//    MulInstruction = InstructionBuilder.CreateFMul(static_cast<const CallInst*>(&(*NextInst))->getArgOperand(0),
//                                                          static_cast<const CallInst*>(&(*NextInst))->getArgOperand(1));
//    NextInst++;
//    AddInstruction = InstructionBuilder.CreateFAdd(MulInstruction,
//                                                   static_cast<const CallInst*>(BaseInstruction)->getArgOperand(2));
////    ReplaceInstWithInst(&(*NextInst),  BinaryOperator::Create(Instruction::FAdd, MulInstruction, static_cast<const CallInst*>(&(*NextInst))->getArgOperand(2), "fadd"));
//    // Replaces all of the uses of the instruction with uses of the value
//    BaseInstruction->replaceAllUsesWith(AddInstruction);

//    assert(ACComputingCallInstruction && CGCallInstruction && "Invalid call instruction!");
  }


  return;
}

void ACInstrumentation::instrumentCallsForNonACIntrinsicFunction(
    Instruction *BaseInstruction, long *NumInstrumentedInstructions) {
  assert((CGCreateNode!=nullptr) && "Function not initialized!");

  BasicBlock::iterator NextInst(BaseInstruction);
  NextInst++;
  IRBuilder<> InstructionBuilder( &(*NextInst) );
  std::vector<Value *> Args;

  CallInst *CGCallInstruction = nullptr;

  Constant *EmptyValue = ConstantDataArray::getString(BaseInstruction->getModule()->getContext(),
                                                      "",
                                                      true);

  Value *EmptyValuePointer = new GlobalVariable(*BaseInstruction->getModule(),
                                                EmptyValue->getType(),
                                                true,
                                                GlobalValue::InternalLinkage,
                                                EmptyValue);

  Value *FileNameValuePointer;
  int LineNumber;
  if(BaseInstruction->getDebugLoc()) {
    FileNameValuePointer = createStringRefGlobalString(
        BaseInstruction->getDebugLoc()->getDirectory(), BaseInstruction);
    LineNumber = BaseInstruction->getDebugLoc().getLine();
  }
  else {
    FileNameValuePointer = EmptyValuePointer;
    LineNumber = -1;
  }

  Args.push_back(createInstructionGlobalString(BaseInstruction));
  Args.push_back(EmptyValuePointer);
  Args.push_back(EmptyValuePointer);
  Args.push_back(InstructionBuilder.getInt32(NodeKind::Register));
  Args.push_back(FileNameValuePointer);
  Args.push_back(InstructionBuilder.getInt32(LineNumber));
  ArrayRef<Value *> ArgsRef(Args);

  CGCallInstruction = InstructionBuilder.CreateCall(CGCreateNode, ArgsRef);
  *NumInstrumentedInstructions+=1;

  assert(CGCallInstruction && "Invalid call instruction!");
  return;
}

// Instruments a call to calculate Amplification Factors at the node corresponding
// this instruction.
void ACInstrumentation::instrumentCallsForAFAnalysis(
    Instruction *BaseInstruction, Instruction *LocationToInstrument,
    long *NumInstrumentedInstructions) {
  assert((AFfp32AnalysisFunction!=nullptr) &&
         (AFfp64AnalysisFunction!=nullptr) &&
         "Function not initialized!");

  // Backtracking till you get a function of interest.
  // NOTE: Assuming following the usedef chain of just the first operands back
  // will give the function of interest
//  while(!isInstructionOfInterest(BaseInstruction) &&
//         static_cast<Instruction*>(BaseInstruction->getOperand(0))) {
//    BaseInstruction = static_cast<Instruction*>(BaseInstruction->getOperand(0));
//  }

  // If Instruction is not of interest, ignore and move ahead.
  if(!isInstructionOfInterest(BaseInstruction))
    return ;

  BasicBlock::iterator NextInst(LocationToInstrument);
  NextInst++;
  IRBuilder<> InstructionBuilder( &(*NextInst) );

  std::vector<Value *> Args;
  Args.push_back(createInstructionGlobalString(BaseInstruction));
  ArrayRef<Value *> ArgsRef(Args);

  CallInst *AFComputingCallInstruction = nullptr;
  if(isSingleFPOperation(BaseInstruction)) {
    AFComputingCallInstruction =
        InstructionBuilder.CreateCall(
            AFfp32AnalysisFunction,
            ArgsRef);
    *NumInstrumentedInstructions+=1;
  }
  else if(isDoubleFPOperation(BaseInstruction)) {
    AFComputingCallInstruction =
        InstructionBuilder.CreateCall(
            AFfp64AnalysisFunction,
            ArgsRef);
    *NumInstrumentedInstructions+=1;
  }

  assert(AFComputingCallInstruction && "Invalid call instruction!");
  return;
}

void ACInstrumentation::instrumentBasicBlock(BasicBlock *BB,
                                             long *NumInstrumentedInstructions) {
  if (ACInstrumentation::isUnwantedFunction(BB->getParent()))
    return;

//  List of Instructions to be Analysed. A call to analyse the computation DAG
//  of this instruction will be instrumented at the end of basic block BB.
  std::vector<Instruction *> InstructionsToAnalyse;

  instrumentCallRecordingPHIInstructions(BB,
                                       NumInstrumentedInstructions);
  instrumentCallRecordingBasicBlock(BB,
                                    NumInstrumentedInstructions);

  for (BasicBlock::iterator I = BB->begin();
       I != BB->end(); ++I) {
    Instruction* CurrentInstruction = &*I;

    // Branch based on kind of Instruction
    if(isMemoryLoadOperation(CurrentInstruction)) {
      instrumentCallsForMemoryLoadOperation(CurrentInstruction,
                                            NumInstrumentedInstructions);
    }
    else if(isIntegerToFloatCastOperation(CurrentInstruction)) {
      instrumentCallsForCastOperation(CurrentInstruction,
                                      NumInstrumentedInstructions);
    }
    // isNonACInstrinsicFunction checks whether the intrinsic function is one from
    // the list that we do not calculate AC for. Only the CG Node.
    else if(CurrentInstruction->getOpcode() == Instruction::Call &&
             isNonACInstrinsicFunction(CurrentInstruction)) {
      instrumentCallsForNonACIntrinsicFunction(CurrentInstruction,
                                               NumInstrumentedInstructions);
    }
    else if(isOtherOperation(CurrentInstruction)) {
      instrumentCallsForOtherOperation(CurrentInstruction,
                                       NumInstrumentedInstructions);
    }
    else if(isUnaryOperation(CurrentInstruction)) {
      instrumentCallsForUnaryOperation(CurrentInstruction,
                                       NumInstrumentedInstructions);
    }
    else if(isBinaryOperation(CurrentInstruction)) {
      instrumentCallsForBinaryOperation(CurrentInstruction,
                                        NumInstrumentedInstructions);
    }

    // Instrument Amplification Factor Calculating Function
//    if(CurrentInstruction->getOpcode() == Instruction::Call) {
//
//      string FunctionName = "";
//      if(static_cast<CallInst*>(CurrentInstruction)->getCalledFunction() &&
//          static_cast<CallInst*>(CurrentInstruction)->getCalledFunction()->hasName())
//        FunctionName = static_cast<CallInst*>(CurrentInstruction)->getCalledFunction()->getName().str();
//      transform(FunctionName.begin(), FunctionName.end(), FunctionName.begin(), ::tolower);
//      if(FunctionName.find("markforresult") != std::string::npos) {
//        if(!isa<Constant>(static_cast<CallInst*>(CurrentInstruction)->data_operands_begin()->get())) {
//          instrumentCallsForAFAnalysis(
//              static_cast<Instruction *>(
//                  static_cast<CallInst *>(CurrentInstruction)
//                      ->data_operands_begin()
//                      ->get()),
//              CurrentInstruction, NumInstrumentedInstructions);
//        } else {
//          errs() << "Value to be analyzed has been optimized into a constant\n";
//        }
//      }
//    }

    // Instrument Amplification Factor Calculating Function in case there is a
    // print function or a return call
    if(CurrentInstruction->getOpcode() == Instruction::Ret ||
        CurrentInstruction->getOpcode() == Instruction::Call) {
      if(CurrentInstruction->getOpcode() == Instruction::Call) {
        string FunctionName = "";
        if(static_cast<CallInst*>(CurrentInstruction)->getCalledFunction() &&
            static_cast<CallInst*>(CurrentInstruction)->getCalledFunction()->hasName())
          FunctionName = static_cast<CallInst*>(CurrentInstruction)->getCalledFunction()->getName().str();
        transform(FunctionName.begin(), FunctionName.end(), FunctionName.begin(), ::tolower);
        if(FunctionName.find("print") != std::string::npos){
          for (Function::op_iterator CurrResultOp = static_cast<CallInst*>(CurrentInstruction)->data_operands_begin();
               CurrResultOp != static_cast<CallInst*>(CurrentInstruction)->data_operands_end();
               ++CurrResultOp) {
//            errs() << "Operand: " << *CurrResultOp << "\n";
            Instruction* ResultInstruction;
            if(isa<Instruction>(CurrResultOp->get())) {
              ResultInstruction =
                  static_cast<Instruction *>(CurrResultOp->get());
              if(ResultInstruction->getType()->isFloatingPointTy()) {
//                while(!isUnaryOperation(ResultInstruction) &&
//                       !isBinaryOperation(ResultInstruction) &&
//                       ResultInstruction->getOpcode() != Instruction::PHI) {
//                  if (isa<Instruction>(ResultInstruction->getOperand(0)))
//                    ResultInstruction =
//                        (Instruction *)ResultInstruction->getOperand(0);
//                  else
//                    exit(1);
//                }
                if (isUnaryOperation(ResultInstruction) ||
    isBinaryOperation(ResultInstruction)) {
                  instrumentCallsForAFAnalysis(ResultInstruction,
                                               CurrentInstruction,
                                               NumInstrumentedInstructions);
                }
              }
            }
          }
        }
      }
      else {
        Instruction* ResultInstruction;
        if(static_cast<ReturnInst*>(CurrentInstruction)->getReturnValue() &&
            isa<Instruction>(static_cast<ReturnInst*>(CurrentInstruction)->getReturnValue())) {
          ResultInstruction =
              static_cast<Instruction *>(static_cast<ReturnInst*>(CurrentInstruction)->getReturnValue());
          if(ResultInstruction->getType()->isFloatingPointTy()) {
            if (isUnaryOperation(ResultInstruction) ||
                isBinaryOperation(ResultInstruction)) {
              instrumentCallsForAFAnalysis(ResultInstruction,
                                           CurrentInstruction,
                                           NumInstrumentedInstructions);
            }
          }
        }
      }
    }
  }

  return;
}

void ACInstrumentation::instrumentMainFunction(Function *F) {
  assert((ACInitFunction!=nullptr) &&
         (CGInitFunction!=nullptr) &&
         (AFInitFunction!= nullptr) &&
         (ACStoreFunction!=nullptr) &&
         (CGStoreFunction!=nullptr) &&
         (AFStoreFunction!= nullptr) &&
         (AFPrintTopAmplificationPaths != nullptr) &&
         (CGDotGraphFunction!=nullptr) &&
         "Function not initialized!");
  BasicBlock *BB = &(*(F->begin()));
  Instruction *Inst = BB->getFirstNonPHIOrDbg();
  IRBuilder<> InstructionBuilder(Inst);
  std::vector<Value *> ACInitCallArgs, CGInitCallArgs, AFInitCallArgs, PrintAFPathsCallArgs;
  std::vector<Value *> ACStoreCallArgs, CGStoreCallArgs, AFStoreCallArgs;
  std::vector<Value *> DotGraphCallArgs;

  CallInst *ACInitCallInstruction, *CGInitCallInstruction, *AFInitCallInstruction, *PrintAFPathsCallInstruction;
  CallInst *StoreACTableCallInstruction, *StoreCGTableCallInstruction,
      *StoreAFTableCallInstruction;
  CallInst *DotGraphCallInstruction;

  // Instrumenting Initialization call instruction
//  Args.push_back(InstructionBuilder.getInt64(1000));
  // Check if function has arguments or not
  // TODO: Handle the case if program has arguments by creating a function
  //  initializing everything in case of arguments.
//  if (F->arg_size() == 2) {
//    // Push parameters
//    for (Argument *I = F->arg_begin(); I != F->arg_end(); ++I) {
//      Value *V = &(*I);
//      Args.push_back(V);
//    }
//    ArrayRef<Value *> args_ref(Args);
//    CallInstruction = InstructionBuilder.CreateCall(fpc_init_args, args_ref);
//  } else
//  {
  ACInitCallInstruction = InstructionBuilder.CreateCall(ACInitFunction, ACInitCallArgs);
  CGInitCallInstruction = InstructionBuilder.CreateCall(CGInitFunction, CGInitCallArgs);
  AFInitCallInstruction = InstructionBuilder.CreateCall(AFInitFunction, AFInitCallArgs);
//  }

  // Instrument call to print table
  for (Function::iterator BBIter=F->begin(); BBIter != F->end(); ++BBIter) {
    for (BasicBlock::iterator InstIter=BBIter->begin(); InstIter != BBIter->end(); ++InstIter) {
      Instruction *CurrentInstruction = &(*InstIter);
      if (isa<ReturnInst>(CurrentInstruction) || isa<ResumeInst>(CurrentInstruction)) {
        ArrayRef<Value *> ACStoreCallArgsRef(ACStoreCallArgs);
        ArrayRef<Value *> CGStoreCallArgsRef(CGStoreCallArgs);
        ArrayRef<Value *> AFStoreCallArgsRef(AFStoreCallArgs);
        ArrayRef<Value *> PrintAFPathsCallArgsRef(PrintAFPathsCallArgs);

        InstructionBuilder.SetInsertPoint(CurrentInstruction);
        StoreACTableCallInstruction = InstructionBuilder.CreateCall(ACStoreFunction, ACStoreCallArgsRef);
        StoreCGTableCallInstruction = InstructionBuilder.CreateCall(CGStoreFunction, CGStoreCallArgsRef);
        StoreAFTableCallInstruction = InstructionBuilder.CreateCall(AFStoreFunction, AFStoreCallArgsRef);
        PrintAFPathsCallInstruction = InstructionBuilder.CreateCall(AFPrintTopAmplificationPaths, PrintAFPathsCallArgsRef);

        ArrayRef<Value *> DotGraphCallArgsRef(DotGraphCallArgs);
        DotGraphCallInstruction = InstructionBuilder.CreateCall(CGDotGraphFunction, DotGraphCallArgsRef);
      }
    }
  }

  assert(ACInitCallInstruction && CGInitCallInstruction &&
         AFInitCallInstruction && "Invalid call instruction!");
  assert(StoreACTableCallInstruction && StoreCGTableCallInstruction &&
         StoreAFTableCallInstruction && PrintAFPathsCallInstruction && "Invalid call instruction!");
  assert(DotGraphCallInstruction && "Invalid call instruction!");
  return;
}

bool ACInstrumentation::isMemoryLoadOperation(const Instruction *Inst) {
  return Inst->getOpcode() == Instruction::Alloca ||
         Inst->getOpcode() == Instruction::Load;
}

bool ACInstrumentation::isIntegerToFloatCastOperation(const Instruction *Inst) {
  return Inst->getOpcode() == Instruction::UIToFP ||
         Inst->getOpcode() == Instruction::SIToFP;
}

bool ACInstrumentation::isUnaryOperation(const Instruction *Inst) {
  return Inst->getOpcode() == Instruction::FNeg ||
         Inst->getOpcode() == Instruction::FPTrunc ||
         (Inst->getOpcode() == Instruction::Call &&
          (static_cast<const CallInst*>(Inst)->getCalledFunction() &&
           isFunctionOfInterest(
              static_cast<const CallInst*>(Inst)->getCalledFunction())));
}

// TODO: Check for FRem case.
bool ACInstrumentation::isBinaryOperation(const Instruction *Inst) {
  return Inst->getOpcode() == Instruction::FAdd ||
         Inst->getOpcode() == Instruction::FSub ||
         Inst->getOpcode() == Instruction::FMul ||
         Inst->getOpcode() == Instruction::FDiv;
}

bool ACInstrumentation::isOtherOperation(const Instruction *Inst) {
  return (Inst->getOpcode() == Instruction::Call &&
          static_cast<const CallInst*>(Inst)->getCalledFunction() &&
          static_cast<const CallInst*>(Inst)->getCalledFunction()->hasName() &&
          (static_cast<const CallInst*>(Inst)->getCalledFunction()->getName().str().find("llvm.fmuladd") !=
              std::string::npos ||
          static_cast<const CallInst*>(Inst)->getCalledFunction()->getName().str().find("llvm.fma") !=
             std::string::npos));
}

bool ACInstrumentation::isNonACInstrinsicFunction(const Instruction *Inst) {
  assert(Inst->getOpcode() == Instruction::Call);
  if(static_cast<const CallInst*>(Inst)->getCalledFunction() &&
      static_cast<const CallInst*>(Inst)->getCalledFunction()->hasName())
    return false;

  return false;
}


bool ACInstrumentation::isSingleFPOperation(const Instruction *Inst) {
  if (isUnaryOperation(Inst)) {
    switch (Inst->getOpcode()) {
    case 12:
      return Inst->getOperand(0)->getType()->isFloatTy();
    case 45:
      return static_cast<const FPTruncInst*>(Inst)->getSrcTy()->isFloatTy();
    case 56:
      // Assuming that operand 0 for this call instruction contains the operand
      // used to calculate the AC.
      return static_cast<const CallInst*>(Inst)->getArgOperand(0)->getType()->isFloatTy();
    default:
      errs() << "Not an FP32 operation.\n";
    }
  } else if(isBinaryOperation(Inst)) {
    return Inst->getOperand(0)->getType()->isFloatTy() &&
           Inst->getOperand(1)->getType()->isFloatTy();
  } else if(Inst->getOpcode() == Instruction::PHI) {
    return static_cast<const PHINode*>(Inst)->getType()->isFloatTy();
  }

  return false;
}

bool ACInstrumentation::isDoubleFPOperation(const Instruction *Inst) {
  if (isUnaryOperation(Inst)) {
    switch (Inst->getOpcode()) {
    case 12:
      return Inst->getOperand(0)->getType()->isDoubleTy();
    case 45:
      return static_cast<const FPTruncInst*>(Inst)->getSrcTy()->isDoubleTy();
    case 56:
      // Assuming that operand 0 for this call instruction contains the operand
      // used to calculate the AC.
      return static_cast<const CallInst*>(Inst)->getArgOperand(0)->getType()->isDoubleTy();
    default:
      errs() << "Not an FP64 operation.\n";
    }
  } else if(isBinaryOperation(Inst)) {
    return Inst->getOperand(0)->getType()->isDoubleTy() &&
           Inst->getOperand(1)->getType()->isDoubleTy();
  } else if(Inst->getOpcode() == Instruction::PHI) {
    return static_cast<const PHINode*>(Inst)->getType()->isDoubleTy();
  }

  return false;
}

bool ACInstrumentation::isUnwantedFunction(const Function *Func) {
  assert(Func->hasName());
  return Func->getName().str().find("fAC") != std::string::npos ||
         Func->getName().str().find("fCG") != std::string::npos ||
         Func->getName().str().find("fAF") != std::string::npos ||
         Func->getName().str().find("fRS") != std::string::npos ||
         Func->getName().str().find("fURT") != std::string::npos ||
         Func->getName().str().find("ACItem") != std::string::npos;
}

bool ACInstrumentation::isFunctionOfInterest(const Function *Func) {
  if(Func->hasName()) {
    string FunctionName = Func->getName().str();
    transform(FunctionName.begin(), FunctionName.end(), FunctionName.begin(),
              ::tolower);
    return FunctionName.find("sin") != std::string::npos ||
           FunctionName.find("cos") != std::string::npos ||
           FunctionName.find("tan") != std::string::npos ||
           FunctionName.find("exp") != std::string::npos ||
           FunctionName.find("log") != std::string::npos ||
           FunctionName.find("sqrt") != std::string::npos ||
           FunctionName.find("llvm.fmuladd") != std::string::npos ||
           FunctionName.find("llvm.fma") != std::string::npos;
  }
  return false;
}

Value *ACInstrumentation::createBBNameGlobalString(BasicBlock *BB) {
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

Value *ACInstrumentation::createRegisterNameGlobalString(Instruction *Inst) {
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

Value *ACInstrumentation::createInstructionGlobalString(Instruction *Inst) {
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

Value *ACInstrumentation::createStringRefGlobalString(StringRef StringObj, Instruction *Inst) {
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

string ACInstrumentation::getInstructionAsString(Instruction *Inst) {
  string InstructionString;
  raw_string_ostream RawInstructionString(InstructionString);
  RawInstructionString << *Inst;
  unsigned long NonEmptyPosition= RawInstructionString.str().find_first_not_of(" \n\r\t\f\v");
  string InstructionAsString = (NonEmptyPosition == std::string::npos) ? "" :
                                                               RawInstructionString.str().substr(NonEmptyPosition);
  return InstructionAsString;
}

bool ACInstrumentation::isInstructionOfInterest(Instruction *Inst) {
  switch (Inst->getOpcode()) {
  case 14:
  case 16:
  case 18:
  case 21:
  case 31:
  case 32:
  case 33:
//  case 45:
  case 55:
  case 56:
    return true;
  }
  return false;
}
