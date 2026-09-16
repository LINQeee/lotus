#include "../../../utils/cutils.h"
#include "../../llvm.h"
#include "../../utils/llvm_extension.h"
#include "../expression/expression_generator.h"
#include "../generator.h"
#include <llvm-c/Core.h>
#include <stdlib.h>

static void generateWhile(const WhileNode *node) {
    LLVMBasicBlockRef condBlock = LLVMAppendBlock("while.cond");
    LLVMBasicBlockRef bodyBlock = LLVMAppendBlock("while.body");
    LLVMBasicBlockRef exitBlock = LLVMAppendBlock("while.exit");

    LLVMBuildBr(cg->builder, condBlock);
    LLVMMoveBuilderToEnd(condBlock);

    LLVMValueRef cond = generateExpression(node->condition);
    LLVMBuildCondBr(cg->builder, cond, bodyBlock, exitBlock);
    LLVMMoveBuilderToEnd(bodyBlock);

    LoopContext loop;
    loop.breakBlock = exitBlock;
    loop.continueBlock = condBlock;
    cg->loops[cg->loopCount++] = loop;

    generateNode(node->body);
    cg->loopCount--;
    if (!LLVMGetBasicBlockTerminator(cg->currentBlock))
        LLVMBuildBr(cg->builder, condBlock);

    LLVMMoveBuilderToEnd(exitBlock);
}

static void generateBreak(const BreakNode *node) {
    int index = cg->loopCount - node->level;
    if (index < 0)
        index = 0;
    LLVMBuildBr(cg->builder, cg->loops[index].breakBlock);
}

static void generateContinue(const ContinueNode *node) {
    int index = cg->loopCount - node->level;
    if (index < 0)
        index = 0;
    LLVMBuildBr(cg->builder, cg->loops[index].continueBlock);
}

static void generateIf(const IfNode *node) {
    LLVMBasicBlockRef thenBlock = LLVMAppendBlock("if.then");
    LLVMBasicBlockRef elseBlock = NULL;
    if (node->elseBranch)
        elseBlock = LLVMAppendBlock("if.else");
    LLVMBasicBlockRef mergeBlock = LLVMAppendBlock("if.merge");

    LLVMValueRef cond = generateExpression(node->condition);
    LLVMBuildCondBr(cg->builder, cond, thenBlock,
                    node->elseBranch ? elseBlock : mergeBlock);

    LLVMMoveBuilderToEnd(thenBlock);
    generateNode(node->thenBranch);
    if (!LLVMGetBasicBlockTerminator(thenBlock))
        LLVMBuildBr(cg->builder, mergeBlock);

    if (node->elseBranch) {
        LLVMMoveBuilderToEnd(elseBlock);
        generateNode(node->elseBranch);
        if (!LLVMGetBasicBlockTerminator(elseBlock))
            LLVMBuildBr(cg->builder, mergeBlock);
    }

    LLVMMoveBuilderToEnd(mergeBlock);
}

static void generateReturn(const ReturnNode *node) {
    if (node->value)
        LLVMBuildRet(cg->builder, generateExpression(node->value));
    else
        LLVMBuildRetVoid(cg->builder);
}

static LLVMTypeRef lotusTypeToLLVMType(LotusType type) {
    switch (type) {
        case LOTUS_I32:
            return cg->i32Type;
        case LOTUS_I64:
            return cg->i64Type;
        case LOTUS_F32:
            return cg->f32Type;
        case LOTUS_F64:
            return cg->f64Type;
        case LOTUS_BOOL:
            return cg->boolType;
        case LOTUS_VOID:
            return cg->voidType;
        case LOTUS_STRING:
            return LLVMPointerType(cg->stringType, 0);

        default:
            return NULL;
    }
}

static void generateDeclaration(const DeclarationNode *node) {
    LLVMValueRef value = generateExpression(node->value);
    LLVMTypeRef type = lotusTypeToLLVMType(node->base.dataType);


    if (cg->currentFn == cg->mainFunction) {
        // TODO: do null pre initialization  only for non-literal types
        LLVMValueRef storage = LLVMAddGlobal(cg->module, type, node->target->name);
        LLVMSetInitializer(storage, LLVMConstNull(type));

        createVar(node->target->name, storage, node->base.dataType);

        LLVMBuildStore(cg->builder, value, storage);
    } else {
        LLVMValueRef storage = LLVMBuildAlloca(cg->builder, type, node->target->name);
        LLVMBuildStore(cg->builder, value, storage);

        createVar(node->target->name, storage, node->base.dataType);
    }
}

static void generateAssignment(const AssignmentNode *node) {
    LLVMVariable *var = getVar(node->target->name);

    if (var == NULL) {
        exitWithError("Variable '%s' is not declared", node->target->name);
    }

    LLVMValueRef value = generateExpression(node->value);

    LLVMBuildStore(cg->builder, value, var->storage);
}

static void generateProgram(ProgramNode *node) {
    for (int i = 0; i < node->count; i++) {
        generateNode(node->statements[i]);
        if (LLVMGetBasicBlockTerminator(cg->currentBlock))
            break;
    }
}

void generateStatement(const Node *node) {
    switch (node->type) {
        case NODE_DECLARATION:
            generateDeclaration((DeclarationNode *) node);
            break;
        case NODE_ASSIGN:
            generateAssignment((AssignmentNode *) node);
            break;
        case NODE_PROGRAM:
            generateProgram((ProgramNode *) node);
            break;
        case NODE_WHILE:
            generateWhile((WhileNode *) node);
            break;
        case NODE_BREAK:
            generateBreak((BreakNode *) node);
            break;
        case NODE_CONTINUE:
            generateContinue((ContinueNode *) node);
            break;
        case NODE_RETURN:
            generateReturn((ReturnNode *) node);
            break;
        case NODE_IF:
            generateIf((IfNode *) node);
            break;
        case NODE_FUNCTION:
            generateFunction((FunctionNode *) node);
            break;
    }
}
