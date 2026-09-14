#include "type_scope.h"

#include <stdlib.h>
#include <string.h>

#include "../utils/cutils.h"

TypeScope *typeScopeCreate(TypeScope *parent) {
    TypeScope *scope = mallocSafe(sizeof(TypeScope));

    scope->count = 0;
    scope->capacity = 8;
    scope->parent = parent;

    scope->variables = mallocSafe(sizeof(Variable) * scope->capacity);
    return scope;
}

void typeScopeDestroy(TypeScope *scope) {
    if (!scope) return;

    for (size_t i = 0; i < scope->count; i++)
        free(scope->variables[i].name);

    free(scope->variables);
    free(scope);
}

void typeScopeDeclare(
    TypeScope *scope,
    const char *name,
    LotusType type
) {
    for (size_t i = 0; i < scope->count; i++) {
        if (strcmp(scope->variables[i].name, name) == 0) {
            exitWithError(
                "Internal type scope error: variable '%s' already exists",
                name
            );
        }
    }

    if (scope->count >= scope->capacity) {
        scope->capacity *= 2;

        scope->variables = reallocSafe(
            scope->variables,
            sizeof(Variable) * scope->capacity
        );
    }

    scope->variables[scope->count++] = (Variable){
        .name = strdup(name),
        .type = type
    };
}

Variable *typeScopeFind(
    TypeScope *scope,
    const char *name
) {
    for (TypeScope *current = scope; current != NULL; current = current->parent) {
        for (size_t i = 0; i < current->count; i++) {
            if (strcmp(current->variables[i].name, name) == 0)
                return &current->variables[i];
        }
    }

    return NULL;
}
