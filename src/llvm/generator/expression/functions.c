#include "native_caller.h"
#include "functions.h"

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <string.h>

#include "../../../parser/parser.h"
#include "../../llvm.h"
#include "native_functions.h"
#include "../../utils/llvm_extension.h"
#include "../generator.h"
#include "expression_generator.h"
#include "../../var_manager.h"
#include "../../lotus_types.h"

void collectFunctions(const Node *node) {
    if (node->type == NODE_FUNCTION) {
        const FunctionNode *fn = (const FunctionNode *)node;

        LLVMUserFunction *f = &cg->functions[cg->functionCount++];

        strcpy(f->name, fn->name);

        f->argc = fn->paramCount;
        f->returnType = fn->base.dataType;

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
            case NODE_PROGRAM: {
                const ProgramNode *program = (const ProgramNode *)node;
                for (int i = 0; i < program->count; i++) collectFunctions(program->statements[i]);
                // TODO collect funcs from entire code
                break;
            }
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
    int previousVarsCount = cg->varsCount;
    int previousFunctionVarsStart = cg->functionVarsStart;
    cg->functionVarsStart = previousVarsCount;

    cg->currentFn = fn->llvmFunction;

    LLVMBasicBlockRef entry = LLVMAppendBlock("entry");

    LLVMMoveBuilderToEnd(entry);

    for (int i = 0; i < fn->argc; i++) {
        LLVMValueRef arg = LLVMGetParam(fn->llvmFunction, i);
        LLVMVariable *var =
            createVar(node->params[i]->name,
                      createEntryAlloca(convertType(node->params[i]->base.dataType), node->params[i]->name),
                      node->params[i]->base.dataType);

        retainIfNeeded(var->type, arg);
        LLVMBuildStore(cg->builder, arg, var->storage);
    }

    generateNode(node->body);

    if (!LLVMGetBasicBlockTerminator(cg->currentBlock)) {
        if (fn->returnType == LOTUS_VOID) {
            releaseVariables(previousVarsCount);
            LLVMBuildRetVoid(cg->builder);
        } else {
            // Type checking proved that a value-returning function can't reach this block.
            LLVMBuildUnreachable(cg->builder);
        }
    }

    cg->varsCount = previousVarsCount;
    cg->functionVarsStart = previousFunctionVarsStart;
    cg->currentBlock = prevCurBlock;
    cg->currentAllocaBlock = prevCurAllocaBlock;
    cg->currentFn = prevFunc;
    LLVMMoveBuilderToEnd(cg->currentBlock);
}

LLVMValueRef generateCall(const CallNode *node) {
    char *name = (node->callee)->name;

    LLVMUserFunction *fn = findFunction(name);

    if (fn) {
        LLVMValueRef args[32];

        for (int i = 0; i < node->argumentCount; i++) {
            args[i] = generateExpression(node->arguments[i]);
        }
        LLVMValueRef result = LLVMBuildCall2(cg->builder, fn->fnType, fn->llvmFunction, args, node->argumentCount,
                                             node->base.dataType == LOTUS_VOID ? "" : "calltmp");
        for (int i = 0; i < node->argumentCount; i++) {
            releaseIfNeeded(node->arguments[i]->dataType, args[i]);
        }
        return result;
    }

    return generateNativeCall(node);
}
