#include "llvm.h"
#include "../native_loader/native_loader.h"
#include "generator/generator.h"
#include "utils/llvm_extension.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Transforms/PassBuilder.h>
#include <mach/mach_time.h>
#include <stdio.h>
#include <stdlib.h>
Codegen *cg;

void initCodegen() {
  cg = malloc(sizeof(Codegen));
  cg->context = LLVMContextCreate();
  cg->module = LLVMModuleCreateWithNameInContext("lotus", cg->context);
  cg->builder = LLVMCreateBuilderInContext(cg->context);
  cg->allocaBuilder = LLVMCreateBuilderInContext(cg->context);

  cg->i64Type = LLVMInt64TypeInContext(cg->context);
  cg->i32Type = LLVMInt32TypeInContext(cg->context);
  cg->boolType = LLVMInt1TypeInContext(cg->context);
  cg->voidType = LLVMVoidTypeInContext(cg->context);
  cg->f32Type = LLVMFloatTypeInContext(cg->context);
  cg->f64Type = LLVMDoubleTypeInContext(cg->context);
  cg->i8Type = LLVMInt8TypeInContext(cg->context);
}

void codegenProgram(const ProgramNode *program) {
  createMainFunctionAndGenerateCode(program);
  verifyCode();
  LLVMLinkInMCJIT();
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  char *ir = LLVMPrintModuleToString(cg->module);
  printf("\n\n###### GENERATED IR:\n%s\n", ir);
  LLVMDisposeMessage(ir);

  optimizeCode();

  LLVMExecutionEngineRef engine;
  char *error = NULL;
  if (LLVMCreateExecutionEngineForModule(&engine, cg->module, &error)) {
    printf("JIT ERROR: %s\n", error);
    LLVMDisposeMessage(error);
    return;
  }
  uint64_t addr = LLVMGetFunctionAddress(engine, "main");
  int (*mainFunc)() = (int (*)())addr;
  printf("###### RUNNING main():\n");
  uint64_t start = mach_absolute_time();
  int result = mainFunc();
  uint64_t end = mach_absolute_time();
  mach_timebase_info_data_t timebase;
  mach_timebase_info(&timebase);
  uint64_t elapsed_ticks = end - start;
  uint64_t elapsed_ms =
      (elapsed_ticks * timebase.numer) / (timebase.denom * 1000000);
  printf("\n\n###### EXECUTED MAIN FUNCTION IN %llu ms:\nExit code: %d\n",
         elapsed_ms, result);

  LLVMPrintModuleToFile(cg->module, "output.ll", &error);

  // link and run executable "clang output.ll -L../libs -llotus_core
  // -Wl,-rpath,@loader_path/../libs -o output && time ./output"
}
