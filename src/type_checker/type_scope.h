#ifndef TYPE_SCOPE_H
#define TYPE_SCOPE_H

#include <stddef.h>

#include "../llvm/types.h"

typedef struct TypeScope {
    Variable *variables;
    size_t count;
    size_t capacity;

    struct TypeScope *parent;
} TypeScope;

TypeScope *typeScopeCreate(TypeScope *parent);
void typeScopeDestroy(TypeScope *scope);

void typeScopeDeclare(TypeScope *scope, const char *name, Type *type);

Variable *typeScopeFind(TypeScope *scope, const char *name);

#endif
