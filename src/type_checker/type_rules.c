#include "type_rules.h"

#include "../lexer/lexer.h"
#include "../llvm/lotus_types.h"

bool isAssignable(LotusType target, LotusType source) {
    return target == source || target == LOTUS_ANY;
}

bool isValidCast(LotusType target, LotusType source) {
    if (!isNumericType(target) || !isNumericType(source)) return false;
    if (target == source) return true;
    switch (source) {
        case LOTUS_I32: return target == LOTUS_I64 || target == LOTUS_F32 || target == LOTUS_F64;
        case LOTUS_I64: return target == LOTUS_F32 || target == LOTUS_F64;
        case LOTUS_F32: return target == LOTUS_F64;
        default: return false;
    }
}

LotusType commonNumericType(LotusType left, LotusType right) {
    return left == right && isNumericType(left) ? left : LOTUS_ANY;
}

bool areComparable(LotusType left, LotusType right) {
    if (left == right && isNumericType(left)) return true;

    if (left == LOTUS_BOOL && right == LOTUS_BOOL) return true;

    if (left == LOTUS_STRING && right == LOTUS_STRING) return true;

    return false;
}

bool isValidBinaryOperation(TokenType operator, LotusType left, LotusType right) {
    if (left != right) return false;

    switch (operator) {
        case TOKEN_PLUS:
            if (isNumericType(left) || left == LOTUS_STRING) return true;
            return false;

        case TOKEN_MINUS:
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_GT:
        case TOKEN_LT:
        case TOKEN_GE:
        case TOKEN_LE: return isNumericType(left);

        case TOKEN_DOUBLE_STAR: return left == LOTUS_F32 || left == LOTUS_F64;

        case TOKEN_PERCENT: return isIntegerType(left);

        case TOKEN_EQ:
        case TOKEN_NEQ: return areComparable(left, right);

        case TOKEN_AND:
        case TOKEN_OR: return left == LOTUS_BOOL;

        default: return false;
    }
}

LotusType binaryResultType(TokenType operator, LotusType left, LotusType right) {
    switch (operator) {
        case TOKEN_PLUS:
            if (left == LOTUS_STRING && right == LOTUS_STRING) return LOTUS_STRING;

            return commonNumericType(left, right);

        case TOKEN_MINUS:
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT: return commonNumericType(left, right);

        case TOKEN_DOUBLE_STAR: return left;

        case TOKEN_GT:
        case TOKEN_LT:
        case TOKEN_GE:
        case TOKEN_LE:
        case TOKEN_EQ:
        case TOKEN_NEQ:
        case TOKEN_AND:
        case TOKEN_OR: return LOTUS_BOOL;

        default: return LOTUS_ANY;
    }
}
