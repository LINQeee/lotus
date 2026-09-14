#ifndef LLVM_EXTENSION_H
#define LLVM_EXTENSION_H
#include <llvm-c/Types.h>

void createMainFunctionAndGenerateCode(const ProgramNode *program);

void optimizeCode();

void LLVMMoveBuilderToEnd(LLVMBasicBlockRef block);

LLVMBasicBlockRef LLVMAppendBlock(const char *name);

void verifyCode();
#endif
