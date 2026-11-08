#include <llvm-c/Core.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../../../utils/cutils.h"
#include "../../llvm.h"
#include "../../utils/llvm_extension.h"
#include "../expression/expression_generator.h"
#include "../generator.h"
#include "../expression/functions.h"
#include "../../lotus_types.h"
#include "../../var_manager.h"
#include "../../../parser/parser.h"
#include <llvm-c/Types.h>

static void generateScopedNode(const Node *node) {
    int previousVarsCount = cg->varsCount;
    generateNode(node);
    if (!LLVMGetBasicBlockTerminator(cg->currentBlock)) releaseVariables(previousVarsCount);
    cg->varsCount = previousVarsCount;
}

typedef struct {
    LLVMTypeRef type;
    LLVMValueRef start;
    LLVMValueRef end;
    LLVMValueRef step;
    LLVMValueRef descending;
    LLVMValueRef nonempty;
} LLVMRange;

static LLVMValueRef buildRangeCondition(LLVMValueRef start, LLVMValueRef end, LLVMValueRef descending) {
    LLVMValueRef forward = LLVMBuildICmp(cg->builder, LLVMIntSLE, start, end, "range.forward");
    LLVMValueRef backward = LLVMBuildICmp(cg->builder, LLVMIntSGE, start, end, "range.backward");
    return LLVMBuildSelect(cg->builder, descending, backward, forward, "range.nonempty");
}

static LLVMRange generateRange(const ForNode *node) {
    LLVMRange range;
    range.start = generateExpression(node->start);
    range.end = generateExpression(node->end);
    range.type = LLVMTypeOf(range.start);
    LLVMValueRef ascending = LLVMBuildICmp(cg->builder, LLVMIntSLT, range.start, range.end, "range.ascending");
    range.step = node->step ? generateExpression(node->step)
                            : LLVMBuildSelect(cg->builder, ascending, LLVMConstInt(range.type, 1, false),
                                              LLVMConstAllOnes(range.type), "range.step");
    range.descending =
        LLVMBuildICmp(cg->builder, LLVMIntSLT, range.step, LLVMConstNull(range.type), "range.descending");
    LLVMValueRef directionMatches = NULL;
    if (node->excludeStart || node->excludeEnd)
        directionMatches = buildRangeCondition(range.start, range.end, range.descending);

    // One extra bit keeps excluded bounds and the next value representable at integer limits.
    LLVMTypeRef wideType = LLVMIntTypeInContext(cg->context, LLVMGetIntTypeWidth(range.type) + 1);
    range.start = LLVMBuildSExt(cg->builder, range.start, wideType, "range.start");
    range.end = LLVMBuildSExt(cg->builder, range.end, wideType, "range.end");
    range.step = LLVMBuildSExt(cg->builder, range.step, wideType, "range.wide_step");

    if (directionMatches) {
        LLVMValueRef unit = LLVMBuildSelect(cg->builder, ascending, LLVMConstInt(wideType, 1, false),
                                            LLVMConstAllOnes(wideType), "range.unit");
        if (node->excludeStart) range.start = LLVMBuildAdd(cg->builder, range.start, unit, "range.first");
        if (node->excludeEnd) range.end = LLVMBuildSub(cg->builder, range.end, unit, "range.last");

        // Excluding adjacent or equal bounds must not turn an empty range around.
        LLVMValueRef boundsDescending = LLVMBuildNot(cg->builder, ascending, "range.bounds_descending");
        LLVMValueRef boundsValid = buildRangeCondition(range.start, range.end, boundsDescending);
        range.nonempty = LLVMBuildAnd(cg->builder, directionMatches, boundsValid, "range.nonempty");
    } else range.nonempty = buildRangeCondition(range.start, range.end, range.descending);
    return range;
}

static LLVMValueRef createRangeVariable(const IdentifierNode *variable, const LLVMRange *range) {
    LLVMBasicBlockRef entry = LLVMGetEntryBasicBlock(cg->currentFn);
    LLVMValueRef first = LLVMGetFirstInstruction(entry);
    if (first) LLVMPositionBuilderBefore(cg->allocaBuilder, first);
    else LLVMPositionBuilderAtEnd(cg->allocaBuilder, entry);
    LLVMValueRef storage = LLVMBuildAlloca(cg->allocaBuilder, range->type, variable->name);
    createVar(variable->name, storage, variable->base.dataType);
    LLVMValueRef initial = LLVMBuildTrunc(cg->builder, range->start, range->type, "range.initial");
    LLVMBuildStore(cg->builder, initial, storage);
    return storage;
}

static void generateRangeStep(const LLVMRange *range, LLVMValueRef storage, LLVMBasicBlockRef bodyBlock,
                              LLVMBasicBlockRef exitBlock) {
    LLVMValueRef current = LLVMBuildLoad2(cg->builder, range->type, storage, "range.current");
    current = LLVMBuildSExt(cg->builder, current, LLVMTypeOf(range->step), "range.wide_current");
    LLVMValueRef next = LLVMBuildAdd(cg->builder, current, range->step, "range.next");
    LLVMValueRef more = buildRangeCondition(next, range->end, range->descending);
    LLVMBasicBlockRef advanceBlock = LLVMAppendBlock("for.advance");
    LLVMBuildCondBr(cg->builder, more, advanceBlock, exitBlock);

    LLVMMoveBuilderToEnd(advanceBlock);
    next = LLVMBuildTrunc(cg->builder, next, range->type, "range.value");
    LLVMBuildStore(cg->builder, next, storage);
    LLVMBuildBr(cg->builder, bodyBlock);
}

static void generateFor(const ForNode *node) {
    int previousVarsCount = cg->varsCount;
    LLVMRange range = generateRange(node);
    LLVMValueRef storage = createRangeVariable(node->variable, &range);
    LLVMBasicBlockRef bodyBlock = LLVMAppendBlock("for.body");
    LLVMBasicBlockRef stepBlock = LLVMAppendBlock("for.step");
    LLVMBasicBlockRef exitBlock = LLVMAppendBlock("for.exit");
    LLVMBuildCondBr(cg->builder, range.nonempty, bodyBlock, exitBlock);

    LLVMMoveBuilderToEnd(bodyBlock);
    LoopContext loop = {.breakBlock = exitBlock, .continueBlock = stepBlock, .varsStart = cg->varsCount};
    if (cg->loopCount >= MAX_LOOP_DEPTH) exitWithError("Too many nested loops");
    cg->loops[cg->loopCount++] = loop;
    generateScopedNode(node->body);
    cg->loopCount--;
    if (!LLVMGetBasicBlockTerminator(cg->currentBlock)) LLVMBuildBr(cg->builder, stepBlock);

    LLVMMoveBuilderToEnd(stepBlock);
    generateRangeStep(&range, storage, bodyBlock, exitBlock);
    LLVMMoveBuilderToEnd(exitBlock);
    cg->varsCount = previousVarsCount;
}

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
    loop.varsStart = cg->varsCount;
    loop.continueBlock = condBlock;
    if (cg->loopCount >= MAX_LOOP_DEPTH) exitWithError("Too many nested loops");
    cg->loops[cg->loopCount++] = loop;

    generateScopedNode(node->body);
    cg->loopCount--;
    if (!LLVMGetBasicBlockTerminator(cg->currentBlock)) LLVMBuildBr(cg->builder, condBlock);

    LLVMMoveBuilderToEnd(exitBlock);
}

static void generateBreak(const BreakNode *node) {
    int index = cg->loopCount - node->level;
    if (index < 0) index = 0;
    releaseVariables(cg->loops[index].varsStart);
    LLVMBuildBr(cg->builder, cg->loops[index].breakBlock);
}

static void generateContinue(const ContinueNode *node) {
    int index = cg->loopCount - node->level;
    if (index < 0) index = 0;
    releaseVariables(cg->loops[index].varsStart);
    LLVMBuildBr(cg->builder, cg->loops[index].continueBlock);
}

static void generateIf(const IfNode *node) {
    LLVMBasicBlockRef thenBlock = LLVMAppendBlock("if.then");
    LLVMBasicBlockRef elseBlock = NULL;
    if (node->elseBranch) elseBlock = LLVMAppendBlock("if.else");
    LLVMBasicBlockRef mergeBlock = LLVMAppendBlock("if.merge");

    LLVMValueRef cond = generateExpression(node->condition);
    LLVMBuildCondBr(cg->builder, cond, thenBlock, node->elseBranch ? elseBlock : mergeBlock);

    LLVMMoveBuilderToEnd(thenBlock);
    generateScopedNode(node->thenBranch);
    bool thenTerminates = LLVMGetBasicBlockTerminator(cg->currentBlock) != NULL;
    if (!thenTerminates) LLVMBuildBr(cg->builder, mergeBlock);

    bool elseTerminates = false;
    if (node->elseBranch) {
        LLVMMoveBuilderToEnd(elseBlock);
        generateScopedNode(node->elseBranch);
        elseTerminates = LLVMGetBasicBlockTerminator(cg->currentBlock) != NULL;
        if (!elseTerminates) LLVMBuildBr(cg->builder, mergeBlock);
    }

    LLVMMoveBuilderToEnd(mergeBlock);
    if (thenTerminates && elseTerminates) LLVMBuildUnreachable(cg->builder);
}

static void generateReturn(const ReturnNode *node) {
    LLVMValueRef value = node->value ? generateExpression(node->value) : NULL;
    releaseVariables(cg->functionVarsStart);
    if (value) LLVMBuildRet(cg->builder, value);
    else LLVMBuildRetVoid(cg->builder);
}

static LLVMTypeRef lotusTypeToLLVMType(LotusType type) {
    switch (type) {
        case LOTUS_I32: return cg->i32Type;
        case LOTUS_I64: return cg->i64Type;
        case LOTUS_F32: return cg->f32Type;
        case LOTUS_F64: return cg->f64Type;
        case LOTUS_BOOL: return cg->boolType;
        case LOTUS_VOID: return cg->voidType;
        case LOTUS_STRING: return LLVMPointerType(cg->stringType, 0);

        default: return NULL;
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
        LLVMBasicBlockRef entry = LLVMGetEntryBasicBlock(cg->currentFn);
        LLVMValueRef first = LLVMGetFirstInstruction(entry);
        if (first) LLVMPositionBuilderBefore(cg->allocaBuilder, first);
        else LLVMPositionBuilderAtEnd(cg->allocaBuilder, entry);
        LLVMValueRef storage = LLVMBuildAlloca(cg->allocaBuilder, type, node->target->name);
        LLVMBuildStore(cg->builder, value, storage);

        createVar(node->target->name, storage, node->base.dataType);
    }
}

static void generateAssignment(const AssignmentNode *node) {
    LLVMVariable *var = getVar(node->target->name);

    if (var == NULL) exitWithError("Variable '%s' is not declared", node->target->name);

    LLVMValueRef value = generateExpression(node->value);
    if (var->type == LOTUS_STRING) {
        LLVMValueRef previous =
            LLVMBuildLoad2(cg->builder, lotusTypeToLLVMType(LOTUS_STRING), var->storage, "str.previous");
        releaseString(previous);
    }
    LLVMBuildStore(cg->builder, value, var->storage);
}

static void generateProgram(ProgramNode *node) {
    for (int i = 0; i < node->count; i++) {
        generateNode(node->statements[i]);
        if (LLVMGetBasicBlockTerminator(cg->currentBlock)) break;
    }
}

void generateStatement(const Node *node) {
    switch (node->type) {
        case NODE_DECLARATION: generateDeclaration((DeclarationNode *)node); break;
        case NODE_ASSIGN: generateAssignment((AssignmentNode *)node); break;
        case NODE_PROGRAM: generateProgram((ProgramNode *)node); break;
        case NODE_FOR: generateFor((ForNode *)node); break;
        case NODE_WHILE: generateWhile((WhileNode *)node); break;
        case NODE_BREAK: generateBreak((BreakNode *)node); break;
        case NODE_CONTINUE: generateContinue((ContinueNode *)node); break;
        case NODE_RETURN: generateReturn((ReturnNode *)node); break;
        case NODE_IF: generateIf((IfNode *)node); break;
        case NODE_FUNCTION: generateFunction((FunctionNode *)node); break;
    }
}
