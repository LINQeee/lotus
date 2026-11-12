#include "native_caller.h"
#include "../expression/expression_generator.h"

#include <llvm-c/Core.h>
#include <string.h>
#include <stdbool.h>
#include <llvm-c/Types.h>

#include "../../../utils/cutils.h"
#include "../../llvm.h"
#include "native_functions.h"
#include "../../../lexer/lexer.h"
#include "../../lotus_types.h"
#include "../../var_manager.h"
#include "../../../parser/parser.h"
#include "functions.h"
#include "arrays.h"

static LLVMValueRef generateNumber(const NumberNode *node) {
    switch (node->base.dataType) {
        case LOTUS_I32: return LLVMConstInt(cg->i32Type, node->i32, 0);
        case LOTUS_I64: return LLVMConstInt(cg->i64Type, node->i64, 0);
        case LOTUS_F32: return LLVMConstReal(cg->f32Type, node->f32);
        case LOTUS_F64: return LLVMConstReal(cg->f64Type, node->f64);
        default: exitWithError("Unknown number datatype %d", node->base.dataType);
    }
}

static LLVMValueRef generateString(const StringNode *node) {
    size_t len = strlen(node->value);

    LLVMValueRef data = LLVMBuildGlobalStringPtr(cg->builder, node->value, "str.data");
    LLVMValueRef length = LLVMConstInt(cg->i64Type, len, 0);
    LLVMValueRef references = LLVMConstInt(cg->i64Type, 0, 0);
    LLVMValueRef fields[] = {length, data, references};

    LLVMValueRef stringStruct = LLVMConstNamedStruct(cg->stringType, fields, 3);
    LLVMValueRef global = LLVMAddGlobal(cg->module, cg->stringType, "str");
    LLVMSetInitializer(global, stringStruct);

    return global;
}

static LLVMValueRef generateBool(const BooleanNode *node) {
    return LLVMConstInt(cg->boolType, node->value, 0);
}

static LLVMValueRef castNumeric(LLVMValueRef value, LotusType from, LotusType to) {
    if (from == to) return value;

    switch (from) {
        case LOTUS_I32:
            if (to == LOTUS_I64) return LLVMBuildSExt(cg->builder, value, cg->i64Type, "int_to_long");
            if (to == LOTUS_F32) return LLVMBuildSIToFP(cg->builder, value, cg->f32Type, "int_to_float");
            if (to == LOTUS_F64) return LLVMBuildSIToFP(cg->builder, value, cg->f64Type, "int_to_double");
            break;

        case LOTUS_I64:
            if (to == LOTUS_F32) return LLVMBuildSIToFP(cg->builder, value, cg->f32Type, "long_to_float");
            if (to == LOTUS_F64) return LLVMBuildSIToFP(cg->builder, value, cg->f64Type, "long_to_double");
            break;

        case LOTUS_F32:
            if (to == LOTUS_F64) return LLVMBuildFPExt(cg->builder, value, cg->f64Type, "float_to_double");
            break;

        default: break;
    }

    exitWithError("Invalid numeric cast %d -> %d", from, to);
}

static LLVMValueRef buildArithmetic(const BinaryNode *bNode, LLVMValueRef left, LLVMValueRef right) {
    if (bNode->op == TOKEN_DOUBLE_STAR) {
        unsigned int intrinsicId = LLVMLookupIntrinsicID("llvm.pow", 8);

        LLVMTypeRef paramTypes[] = {convertType(bNode->base.dataType)};
        LLVMValueRef func = LLVMGetIntrinsicDeclaration(cg->module, intrinsicId, paramTypes, 1);

        LLVMValueRef args[] = {left, right};
        return LLVMBuildCall2(cg->builder, LLVMGlobalGetValueType(func), func, args, 2, "pow_call");
    }

    switch (bNode->base.dataType) {
        case LOTUS_I32:
        case LOTUS_I64:
            switch (bNode->op) {
                case TOKEN_PLUS: return LLVMBuildAdd(cg->builder, left, right, "add_tmp");
                case TOKEN_MINUS: return LLVMBuildSub(cg->builder, left, right, "sub_tmp");
                case TOKEN_STAR: return LLVMBuildMul(cg->builder, left, right, "mul_tmp");
                case TOKEN_SLASH: return LLVMBuildSDiv(cg->builder, left, right, "div_tmp");
                case TOKEN_PERCENT: return LLVMBuildSRem(cg->builder, left, right, "rem_tmp");
                default: break;
            }
            break;

        case LOTUS_F32:
        case LOTUS_F64:
            switch (bNode->op) {
                case TOKEN_PLUS: return LLVMBuildFAdd(cg->builder, left, right, "add_tmp");
                case TOKEN_MINUS: return LLVMBuildFSub(cg->builder, left, right, "sub_tmp");
                case TOKEN_STAR: return LLVMBuildFMul(cg->builder, left, right, "mul_tmp");
                case TOKEN_SLASH: return LLVMBuildFDiv(cg->builder, left, right, "div_tmp");
                default: break;
            }
            break;

        default: break;
    }

    return NULL;
}

static LLVMValueRef buildComparison(const BinaryNode *bNode, LLVMValueRef left, LLVMValueRef right) {
    LotusType commonType = bNode->left->dataType;
    if (commonType == LOTUS_STRING) {
        LLVMValueRef equal = callStringEquals(left, right);
        releaseIfNeeded(LOTUS_STRING, left);
        releaseIfNeeded(LOTUS_STRING, right);
        return bNode->op == TOKEN_EQ ? equal : LLVMBuildNot(cg->builder, equal, "str.not_equal");
    }

    bool isFloat = commonType == LOTUS_F32 || commonType == LOTUS_F64;

    if (isFloat) {
        LLVMRealPredicate predicate;

        switch (bNode->op) {
            case TOKEN_LT: predicate = LLVMRealOLT; break;
            case TOKEN_GT: predicate = LLVMRealOGT; break;
            case TOKEN_LE: predicate = LLVMRealOLE; break;
            case TOKEN_GE: predicate = LLVMRealOGE; break;
            case TOKEN_EQ: predicate = LLVMRealOEQ; break;
            case TOKEN_NEQ: predicate = LLVMRealONE; break;

            default: return NULL;
        }

        return LLVMBuildFCmp(cg->builder, predicate, left, right, "cmp_tmp");
    }

    LLVMIntPredicate predicate;

    switch (bNode->op) {
        case TOKEN_LT: predicate = LLVMIntSLT; break;
        case TOKEN_GT: predicate = LLVMIntSGT; break;
        case TOKEN_LE: predicate = LLVMIntSLE; break;
        case TOKEN_GE: predicate = LLVMIntSGE; break;
        case TOKEN_EQ: predicate = LLVMIntEQ; break;
        case TOKEN_NEQ: predicate = LLVMIntNE; break;

        default: return NULL;
    }

    return LLVMBuildICmp(cg->builder, predicate, left, right, "cmp_tmp");
}

static LLVMValueRef generateBinary(const BinaryNode *node) {
    LLVMValueRef left = generateExpression(node->left);
    LLVMValueRef right = generateExpression(node->right);

    switch (node->op) {
        case TOKEN_PLUS:
            if (node->base.dataType == LOTUS_STRING) {
                LLVMValueRef result = callStringConcat(left, right);
                releaseIfNeeded(LOTUS_STRING, left);
                releaseIfNeeded(LOTUS_STRING, right);
                return result;
            }

            return buildArithmetic(node, left, right);

        case TOKEN_MINUS:
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_DOUBLE_STAR:
        case TOKEN_PERCENT: return buildArithmetic(node, left, right);

        case TOKEN_LT:
        case TOKEN_GT:
        case TOKEN_LE:
        case TOKEN_GE:
        case TOKEN_EQ:
        case TOKEN_NEQ: return buildComparison(node, left, right);

        case TOKEN_AND: return LLVMBuildAnd(cg->builder, left, right, "andtmp");

        case TOKEN_OR: return LLVMBuildOr(cg->builder, left, right, "ortmp");
    }

    return NULL;
}

static LLVMValueRef generateIdentifier(const char *name) {
    LLVMVariable *var = getVar(name);
    LLVMValueRef value = LLVMBuildLoad2(cg->builder, convertType(var->type), var->storage, name);
    retainIfNeeded(var->type, value);
    return value;
}

static LLVMValueRef generateUpdate(const UnaryNode *node) {
    LLVMVariable *var = getVar(((IdentifierNode *)node->operand)->name);
    LLVMTypeRef type = convertType(var->type);
    LLVMValueRef value = LLVMBuildLoad2(cg->builder, type, var->storage, "update_old");
    bool increment = node->op == TOKEN_INCREMENT;
    LLVMValueRef updated;
    if (var->type == LOTUS_F32 || var->type == LOTUS_F64) {
        LLVMValueRef one = LLVMConstReal(type, 1);
        updated =
            increment ? LLVMBuildFAdd(cg->builder, value, one, "inc") : LLVMBuildFSub(cg->builder, value, one, "dec");
    } else {
        LLVMValueRef one = LLVMConstInt(type, 1, 0);
        updated =
            increment ? LLVMBuildAdd(cg->builder, value, one, "inc") : LLVMBuildSub(cg->builder, value, one, "dec");
    }
    LLVMBuildStore(cg->builder, updated, var->storage);
    return node->postfix ? value : updated;
}

LLVMValueRef generateExpression(const Node *node) {
    switch (node->type) {
        case NODE_ARRAY: return generateArray((const ArrayNode *)node);
        case NODE_LENGTH: return generateLength((const LengthNode *)node);
        case NODE_INDEX:
        case NODE_INDEX_ASSIGN: return generateIndex((const IndexNode *)node);
        case NODE_UNARY: {
            const UnaryNode *unary = (const UnaryNode *)node;
            if (unary->op == TOKEN_INCREMENT || unary->op == TOKEN_DECREMENT) return generateUpdate(unary);
            LLVMValueRef value = generateExpression(unary->operand);
            if (unary->op == TOKEN_EXCLAMATION) return LLVMBuildNot(cg->builder, value, "not_tmp");
            if (node->dataType == LOTUS_F32 || node->dataType == LOTUS_F64)
                return LLVMBuildFNeg(cg->builder, value, "neg_tmp");
            return LLVMBuildNeg(cg->builder, value, "neg_tmp");
        }
        case NODE_CAST: {
            const CastNode *cast = (const CastNode *)node;
            return castNumeric(generateExpression(cast->operand), cast->operand->dataType, node->dataType);
        }
        case NODE_NUMBER: return generateNumber((NumberNode *)node);
        case NODE_BINARY: return generateBinary((BinaryNode *)node);
        case NODE_IDENTIFIER: return generateIdentifier(((IdentifierNode *)node)->name);
        case NODE_CALL: return generateCall((CallNode *)node);
        case NODE_STRING: return generateString((StringNode *)node);
        case NODE_BOOLEAN: return generateBool((BooleanNode *)node);
    }
    return NULL;
}
