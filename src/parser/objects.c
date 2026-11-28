#include "../type_checker/diagnostics.h"
#include "objects.h"

#include <string.h>
#include <stdio.h>

#include "parser_utils.h"
#include "parser_expressions.h"
#include "parser_statements.h"
#include "../utils/cutils.h"
#include "../lexer/lexer.h"
#include "../llvm/types.h"

ObjectType **objectTypes;
size_t objectTypeCount;

bool isObjectType(Type *type) {
    return type->kind == TYPE_OBJECT;
}
ObjectType *getObjectType(Type *type) {
    return type->object;
}
Type *findObjectType(const char *name) {
    for (size_t i = 0; i < objectTypeCount; i++)
        if (!strcmp(objectTypes[i]->name, name)) return &objectTypes[i]->type;
    return unknownType;
}
void collectObjectTypes(const Token *tokens) {
    for (size_t i = 0; tokens[i].type != TOKEN_EOF; i++) {
        if (tokens[i].type != TOKEN_OBJECT_TYPE || tokens[i + 1].type != TOKEN_IDENTIFIER) continue;
        if (findObjectType(tokens[i + 1].value) != unknownType) continue;
        ObjectType *type = newTypeNode(tokens[i + 1].value, tokens[i].location);
        objectTypes = reallocSafe(objectTypes, (objectTypeCount + 1) * sizeof(ObjectType *));
        objectTypes[objectTypeCount++] = type;
    }
}
Node *parseObjectDeclaration() {
    expect(TOKEN_OBJECT_TYPE);
    Token name = expect(TOKEN_IDENTIFIER);
    ObjectType *type = getObjectType(findObjectType(name.value));
    if (type->defined) diagnosticError(parserSource, name.location, "Class '%s' is already declared", name.value);
    type->defined = true;
    expect(TOKEN_LBRACE);
    while (!isCurrent(TOKEN_RBRACE)) {
        if (isCurrent(TOKEN_FUNCTION)) {
            FunctionNode *method = (FunctionNode *)parseStatement();
            if (method->paramCount > 31)
                diagnosticError(parserSource, method->base.location, "Methods support at most 31 parameters");
            memmove(method->params + 1, method->params, method->paramCount * sizeof(IdentifierNode *));
            method->params[0] = newIdentifierNode("this", type->base.dataType, method->base.location);
            method->paramCount++;
            type->methods = reallocSafe(type->methods, (type->methodCount + 1) * sizeof(FunctionNode *));
            type->methods[type->methodCount++] = method;
        } else {
            Token field = expect(TOKEN_IDENTIFIER);
            expect(TOKEN_COLON);
            Type *fieldType = parseType();
            Node *value = matchCurrent(TOKEN_EQUAL) ? parseExpression() : NULL;
            type->fields = reallocSafe(type->fields, (type->fieldCount + 1) * sizeof(ObjectField));
            type->fields[type->fieldCount++] = (ObjectField){field.value, fieldType, value, NULL, field.location};
        }
    }
    expect(TOKEN_RBRACE);
    return (Node *)type;
}
Node *parseObject(Type *type, SourceLocation location) {
    expect(TOKEN_LBRACE);
    ObjectNode *node = newObjectNode(type, location);
    while (!isCurrent(TOKEN_RBRACE)) {
        Token field = expect(TOKEN_IDENTIFIER);
        expect(TOKEN_COLON);
        Node *value = parseExpression();
        node->names = reallocSafe(node->names, (node->count + 1) * sizeof(char *));
        node->values = reallocSafe(node->values, (node->count + 1) * sizeof(Node *));
        node->indices = reallocSafe(node->indices, (node->count + 1) * sizeof(size_t));
        node->names[node->count] = field.value;
        node->values[node->count++] = value;
        if (!matchCurrent(TOKEN_COMMA)) break;
    }
    expect(TOKEN_RBRACE);
    return (Node *)node;
}
void appendObjectFunctions(ProgramNode *program) {
    for (size_t i = 0; i < objectTypeCount; i++) {
        ObjectType *type = objectTypes[i];
        for (size_t j = 0; j < type->methodCount; j++) {
            FunctionNode *method = type->methods[j];
            char name[64];
            int length = snprintf(name, sizeof(name), "%s.%s", type->name, method->name);
            if (length >= (int)sizeof(name))
                diagnosticError(parserSource, method->base.location, "Qualified method name is too long");
            strcpy(method->name, name);
            programAddStatement(program, (Node *)method);
        }
        for (size_t j = 0; j < type->fieldCount; j++) {
            ObjectField *field = &type->fields[j];
            if (!field->defaultValue) continue;
            char name[64];
            snprintf(name, sizeof(name), "$default.%zu.%zu", i, j);
            field->initializer = newFunctionNode(name, field->type, field->location);
            field->initializer->paramCount = 0;
            field->initializer->params = NULL;
            field->initializer->body = (Node *)newReturnNode(field->defaultValue, field->location);
            programAddStatement(program, (Node *)field->initializer);
        }
    }
}
