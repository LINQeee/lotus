#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include <stdbool.h>
#include <stddef.h>

#include "../llvm/types.h"
#include "../parser/parser.h"

typedef struct TypeChecker TypeChecker;

typedef struct {
    char *name;

    Type *returnType;

    Type **parameterTypes;
    size_t parameterCount;

    bool variadic;
} TypeFunction;

TypeChecker *typeCheckerCreate(const char *fileContent);

void typeCheckerDestroy(TypeChecker *checker);

void typeCheckerAddNativeFunction(const char *name, Type *returnType, Type *const *parameterTypes,
                                  size_t parameterCount, bool variadic);

void typeCheckerCheckProgram(TypeChecker *checker, const ProgramNode *program);

#endif
