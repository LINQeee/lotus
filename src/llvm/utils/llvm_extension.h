#ifndef LLVM_EXTENSION_H
#define LLVM_EXTENSION_H
#include <llvm-c/Types.h>
#include <llvm-c/TargetMachine.h>

#include "../../parser/parser.h"

void createMainFunctionAndGenerateCode(const ProgramNode *program);

void optimizeCode();
LLVMTargetMachineRef getTargetMachine();

void LLVMMoveBuilderToEnd(LLVMBasicBlockRef block);

LLVMBasicBlockRef LLVMAppendBlock(const char *name);

void verifyCode();
#endif
