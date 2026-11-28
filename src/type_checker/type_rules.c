#include "type_rules.h"

#include "../lexer/lexer.h"
#include "../llvm/types.h"

bool isAssignable(Type *target, Type *source) {
    return typesEqual(target, source) || target == anyType;
}

bool isValidCast(Type *target, Type *source) {
    if (!isNumericType(target) || !isNumericType(source)) return false;
    if (typesEqual(target, source)) return true;
    switch (source->kind) {
        case TYPE_I32: return target == longType || target == floatType || target == doubleType;
        case TYPE_I64: return target == floatType || target == doubleType;
        case TYPE_F32: return target == doubleType;
        default: return false;
    }
}

Type *commonNumericType(Type *left, Type *right) {
    return typesEqual(left, right) && isNumericType(left) ? left : anyType;
}

bool areComparable(Type *left, Type *right) {
    if (typesEqual(left, right) && isNumericType(left)) return true;

    if (left == boolType && right == boolType) return true;

    if (left == stringType && right == stringType) return true;

    return false;
}

bool isValidBinaryOperation(TokenKind operator, Type *left, Type *right) {
    if (!typesEqual(left, right)) return false;

    switch (operator) {
        case TOKEN_PLUS:
            if (isNumericType(left) || left == stringType) return true;
            return false;

        case TOKEN_MINUS:
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_GT:
        case TOKEN_LT:
        case TOKEN_GE:
        case TOKEN_LE: return isNumericType(left);

        case TOKEN_DOUBLE_STAR: return left == floatType || left == doubleType;

        case TOKEN_PERCENT: return isIntegerType(left);

        case TOKEN_EQ:
        case TOKEN_NEQ: return areComparable(left, right);

        case TOKEN_AND:
        case TOKEN_OR: return left == boolType;

        default: return false;
    }
}

Type *binaryResultType(TokenKind operator, Type *left, Type *right) {
    switch (operator) {
        case TOKEN_PLUS:
            if (left == stringType && right == stringType) return stringType;

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
        case TOKEN_OR: return boolType;

        default: return anyType;
    }
}
