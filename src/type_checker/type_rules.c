#include "type_rules.h"

static int numericRank(LotusType type) {
    switch (type) {
        case LOTUS_I32: return 0;

        case LOTUS_I64: return 1;

        case LOTUS_F32: return 2;

        case LOTUS_F64: return 3;

        default: return -1;
    }
}

bool isAssignable(LotusType target, LotusType source) {
    if (target == source) return true;

    if (target == LOTUS_ANY) return true;

    if (source == LOTUS_ANY) return false;

    if (target == LOTUS_I64 && source == LOTUS_I32) return true;

    if (target == LOTUS_F64 && source == LOTUS_F32) return true;
    //
    // if (target == LOTUS_F32 && source == LOTUS_I32)
    //   return true;
    //
    // if (target == LOTUS_F64 && (source == LOTUS_I32 || source == LOTUS_I64))
    //   return true;

    return false;
}

LotusType commonNumericType(LotusType left, LotusType right) {
    if (!isNumericType(left) || !isNumericType(right)) return LOTUS_ANY;

    return numericRank(left) >= numericRank(right) ? left : right;
}

bool areComparable(LotusType left, LotusType right) {
    if (isNumericType(left) && isNumericType(right)) return true;

    if (left == LOTUS_BOOL && right == LOTUS_BOOL) return true;

    if (left == LOTUS_STRING && right == LOTUS_STRING) return true;

    return false;
}

bool isValidBinaryOperation(TokenType operator, LotusType left, LotusType right) {
    switch (operator) {
        case TOKEN_PLUS:
            if (isNumericType(left) && isNumericType(right)) return true;
            if (left == LOTUS_STRING && right == LOTUS_STRING) return true;

            return false;

        case TOKEN_MINUS:
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_GT:
        case TOKEN_LT:
        case TOKEN_GE:
        case TOKEN_LE: return isNumericType(left) && isNumericType(right);

        case TOKEN_PERCENT: return isIntegerType(left) && isIntegerType(right);

        case TOKEN_EQ:
        case TOKEN_NEQ: return areComparable(left, right);

        case TOKEN_AND:
        case TOKEN_OR: return left == LOTUS_BOOL && right == LOTUS_BOOL;

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
