#ifndef TYPE_RULES_H
#define TYPE_RULES_H

#include <stdbool.h>

#include "../lexer/lexer.h"
#include "../llvm/types.h"

bool isNumericType(Type *type);

bool isIntegerType(Type *type);

bool isBooleanType(Type *type);

bool isAssignable(Type *target, Type *source);

Type *commonNumericType(Type *left, Type *right);

bool areComparable(Type *left, Type *right);

bool isValidBinaryOperation(TokenKind operator, Type * left, Type *right);

Type *binaryResultType(TokenKind operator, Type * left, Type *right);

bool isValidCast(Type *target, Type *source);
#endif
