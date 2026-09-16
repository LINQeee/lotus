#include "functions.h"
#include "../../../parser/parser.h"
#include "../../llvm.h"
#include "native_functions.h"
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include "../../utils/llvm_extension.h"
#include "../generator.h"
#include "expression_generator.h"

void collectFunctions(Node *node) {
    if (node->type == NODE_FUNCTION) {
        FunctionNode *fn = (FunctionNode *)node;

        LLVMUserFunction *f = &cg->functions[cg->functionCount++];

        strcpy(f->name, fn->name);

        f->argc = fn->paramCount;

        LLVMTypeRef params[32];

        for (int i = 0; i < fn->paramCount; i++) {
            f->args[i] = fn->params[i]->base.dataType;

            params[i] = convertType(fn->params[i]->base.dataType);
        }

        LLVMTypeRef type = LLVMFunctionType(convertType(fn->base.dataType), params, f->argc, 0);

        f->fnType = type;

        f->llvmFunction = LLVMAddFunction(cg->module, f->name, type);
    } else {
        switch (node->type) {
            case NODE_PROGRAM:
                ProgramNode *program = (ProgramNode *)node;
                for (int i = 0; i < program->count; i++) collectFunctions(program->statements[i]);
                // TODO collect funcs from entire code
                break;
        }
    }
}

LLVMUserFunction *findFunction(const char *name) {
    for (int i = 0; i < cg->functionCount; i++) {
        if (strcmp(cg->functions[i].name, name) == 0) return &cg->functions[i];
    }

    return NULL;
}

void generateFunction(const FunctionNode *node) {
    LLVMUserFunction *fn = findFunction(node->name);

    LLVMBasicBlockRef prevCurBlock = cg->currentBlock;
    LLVMBasicBlockRef prevCurAllocaBlock = cg->currentAllocaBlock;
    LLVMValueRef prevFunc = cg->currentFn;

    cg->currentFn = fn->llvmFunction;

    LLVMBasicBlockRef entry = LLVMAppendBlock("entry");

    LLVMMoveBuilderToEnd(entry);
    LLVMPositionBuilderAtEnd(cg->allocaBuilder, entry);

    for (int i = 0; i < fn->argc; i++) {
        LLVMValueRef arg = LLVMGetParam(fn->llvmFunction, i);
        LLVMVariable *var = createVar(
            node->params[i]->name,
            LLVMBuildAlloca(cg->allocaBuilder, convertType(node->params[i]->base.dataType), node->params[i]->name),
            node->params[i]->base.dataType);

        LLVMBuildStore(cg->builder, arg, var->storage);
    }

    generateNode(node->body);

    if (!LLVMGetBasicBlockTerminator(entry)) {
        // LLVMBuildRet(cg->builder,LLVMConstInt(cg->i32Type, 0, 0));
        LLVMBuildRetVoid(cg->builder);
    }

    cg->currentBlock = prevCurBlock;
    cg->currentAllocaBlock = prevCurAllocaBlock;
    cg->currentFn = prevFunc;
    LLVMMoveBuilderToEnd(cg->currentBlock);
}

LLVMValueRef generateCall(const CallNode *node) {
    char *name = (node->callee)->name;
    printf("%s\n", name);

    LLVMUserFunction *fn = findFunction(name);

    if (fn) {
        LLVMValueRef args[32];

        for (int i = 0; i < node->argumentCount; i++) {
            args[i] = generateExpression(node->arguments[i]);
        }
        return LLVMBuildCall2(cg->builder, fn->fnType, fn->llvmFunction, args, node->argumentCount,
                              node->base.dataType == LOTUS_VOID ? "" : "calltmp");
    }

    LLVMNativeFunction *native = findNative(name);

    if (native) return generateNativeCall(node);

    printf("Unknown function %s\n", name);
    exit(1);
}
