#ifndef OBJECTS_H
#define OBJECTS_H
#include <stdbool.h>
#include <stddef.h>

#include "parser.h"
#include "../lexer/lexer.h"
#include "../llvm/types.h"

extern ObjectType **objectTypes;
extern size_t objectTypeCount;
bool isObjectType(Type *type);
ObjectType *getObjectType(Type *type);
Type *findObjectType(const char *name);
void collectObjectTypes(const Token *tokens);
Node *parseObjectDeclaration();
Node *parseObject(Type *type, SourceLocation location);
void appendObjectFunctions(ProgramNode *program);
#endif
