#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "../lexer/lexer.h"
#include "../llvm/lotus_types.h"
#include "../parser/parser.h"
#include <stdbool.h>

typedef struct TypeChecker TypeChecker;

typedef struct {
    char *name;

    LotusType returnType;

    LotusType *parameterTypes;
    size_t parameterCount;

    bool variadic;
} TypeFunction;

TypeChecker *typeCheckerCreate(const char *fileContent);

void typeCheckerDestroy(TypeChecker *checker);

void typeCheckerAddNativeFunction(const char *name, LotusType returnType, const LotusType *parameterTypes,
                                  size_t parameterCount, bool variadic);

void typeCheckerCheckProgram(TypeChecker *checker, const ProgramNode *program);

#endif
