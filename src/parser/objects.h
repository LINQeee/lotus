#ifndef OBJECTS_H
#define OBJECTS_H
#include <stdbool.h>
#include <stddef.h>

#include "parser.h"
#include "../lexer/lexer.h"
#include "../llvm/types.h"

extern ClassNode **classes;
extern size_t classCount;
bool isClassType(Type *type);
ClassNode *getClass(Type *type);
Type *findClassType(const char *name);
void collectClasses(const Token *tokens);
Node *parseClassDeclaration();
Node *parseObject(Type *type, SourceLocation location);
void appendClassFunctions(ProgramNode *program);
#endif
