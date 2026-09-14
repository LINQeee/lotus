#ifndef TYPE_RULES_H
#define TYPE_RULES_H

#include <stdbool.h>

#include "../lexer/lexer.h"
#include "../llvm/lotus_types.h"

bool isNumericType(LotusType type);

bool isIntegerType(LotusType type);

bool isBooleanType(LotusType type);

bool isAssignable(
    LotusType target,
    LotusType source
);

LotusType commonNumericType(
    LotusType left,
    LotusType right
);

bool areComparable(
    LotusType left,
    LotusType right
);

bool isValidBinaryOperation(
    TokenType operator,
    LotusType left,
    LotusType right
);

LotusType binaryResultType(
    TokenType operator,
    LotusType left,
    LotusType right
);

#endif
