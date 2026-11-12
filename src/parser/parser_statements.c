#include <stdlib.h>
#include <stdbool.h>

#include "../utils/cutils.h"
#include "./parser.h"
#include "./parser_utils.h"
#include "parser_expressions.h"
#include "../lexer/lexer.h"
#include "../llvm/lotus_types.h"

Node *parseStatement();

static Node *parseBlock() {
    if (!isCurrent(TOKEN_LBRACE)) return parseStatement();
    expect(TOKEN_LBRACE);

    ProgramNode *program = newProgramNode();

    while (!isCurrent(TOKEN_RBRACE) && !isCurrent(TOKEN_EOF)) programAddStatement(program, parseStatement());

    expect(TOKEN_RBRACE);
    return (Node *)program;
}
static Node *parseReturn() {
    SourceLocation returnLoc = expect(TOKEN_RETURN).location;
    if (isCurrent(TOKEN_RBRACE) || isCurrent(TOKEN_EOF) || isCurrent(TOKEN_ELSE) ||
        current().location.line > returnLoc.line)
        return (Node *)newReturnNode(NULL, returnLoc);
    return (Node *)newReturnNode(parseExpression(), returnLoc);
}
static Node *parseBreak() {
    SourceLocation breakLoc = expect(TOKEN_BREAK).location;
    BreakNode *node = newBreakNode(breakLoc);
    if (isCurrent(TOKEN_AT) && peekNext().type == TOKEN_NUMBER) {
        advance(); // skip '@'
        node->level = strtol(advance().value, NULL, 10);
    }
    return (Node *)node;
}
static Node *parseContinue() {
    SourceLocation continueLoc = expect(TOKEN_CONTINUE).location;
    ContinueNode *node = newContinueNode(continueLoc);
    if (isCurrent(TOKEN_AT) && peekNext().type == TOKEN_NUMBER) {
        advance(); // skip '@'
        node->level = strtol(advance().value, NULL, 10);
    }
    return (Node *)node;
}
static Node *parseFor() {
    ForNode *node = newForNode(expect(TOKEN_FOR).location);
    bool parenthesized = matchCurrent(TOKEN_LPAREN);
    Token name = expect(TOKEN_IDENTIFIER);
    node->variable = newIdentifierNode(name.value, LOTUS_UNKNOWN, name.location);
    expect(TOKEN_IN);
    node->start = parseExpression();
    if (!isCurrent(TOKEN_RANGE) && !isCurrent(TOKEN_EXCLAMATION)) {
        if (parenthesized) expect(TOKEN_RPAREN);
        node->body = parseBlock();
        return (Node *)node;
    }
    node->excludeStart = matchCurrent(TOKEN_EXCLAMATION);
    expect(TOKEN_RANGE);
    node->excludeEnd = matchCurrent(TOKEN_EXCLAMATION);
    node->end = parseExpression();
    if (matchCurrent(TOKEN_STEP)) node->step = parseExpression();
    if (parenthesized) expect(TOKEN_RPAREN);
    node->body = parseBlock();
    return (Node *)node;
}

static Node *parseWhile() {
    SourceLocation whileLoc = expect(TOKEN_WHILE).location;
    return (Node *)newWhileNode(parseExpression(), parseBlock(), whileLoc);
}
static Node *parseIf() {
    SourceLocation ifLoc = expect(TOKEN_IF).location;
    IfNode *node = newIfNode(parseExpression(), parseBlock(), ifLoc);
    if (matchCurrent(TOKEN_ELSE)) node->elseBranch = parseBlock();

    return (Node *)node;
}
static Node *parseDeclaration() {
    LotusType type = parseType();

    Token varName = expect(TOKEN_IDENTIFIER);
    expect(TOKEN_EQUAL);

    Node *value = parseExpression();

    IdentifierNode *identifier = newIdentifierNode(varName.value, type, varName.location);

    return (Node *)newDeclarationNode(identifier, value, varName.location);
}
static Node *parseAssignment() {
    Token varName = advance();

    IdentifierNode *identifier = newIdentifierNode(varName.value, LOTUS_UNKNOWN, varName.location);

    Node *value;

    if (matchCurrent(TOKEN_EQUAL)) value = parseExpression();
    else {
        TokenKind op;
        switch (current().type) {
            case TOKEN_EQ_PLUS: op = TOKEN_PLUS; break;
            case TOKEN_EQ_MINUS: op = TOKEN_MINUS; break;
            case TOKEN_EQ_STAR: op = TOKEN_STAR; break;
            case TOKEN_EQ_SLASH: op = TOKEN_SLASH; break;
            case TOKEN_EQ_PERCENT: op = TOKEN_PERCENT; break;
            default: exitWithError("Unknown assignment operator: %d", current().type);
        }
        SourceLocation assignOpLoc = advance().location;
        value = (Node *)newBinaryNode(op, (Node *)identifier, parseExpression(), assignOpLoc);
    }
    return (Node *)newAssignmentNode(identifier, value, varName.location);
}

static IdentifierNode *parseParameter() {
    Token varName = expect(TOKEN_IDENTIFIER);
    expect(TOKEN_COLON);

    LotusType type = parseType();

    return newIdentifierNode(varName.value, type, varName.location);
}

static Node *parseFunction() {
    expect(TOKEN_FUNCTION);

    Token funcName = expect(TOKEN_IDENTIFIER);
    expect(TOKEN_LPAREN);

    IdentifierNode **params = malloc(sizeof(IdentifierNode *) * MAX_ARGS);
    int paramCount = 0;
    if (!isCurrent(TOKEN_RPAREN)) {
        do {
            if (paramCount >= MAX_ARGS) exitWithError("Too many function parameters");

            params[paramCount++] = parseParameter();
        } while (matchCurrent(TOKEN_COMMA));
    }
    expect(TOKEN_RPAREN);

    LotusType returnType = LOTUS_VOID;

    if (matchCurrent(TOKEN_COLON)) returnType = parseType();

    FunctionNode *func = newFunctionNode(funcName.value, returnType, funcName.location);

    func->paramCount = paramCount;
    func->params = params;
    func->body = parseBlock();
    return (Node *)func;
}

Node *parseStatement() {
    switch (current().type) {
        case TOKEN_FUNCTION: return parseFunction();

        case TOKEN_RETURN: return parseReturn();

        case TOKEN_BREAK: return parseBreak();

        case TOKEN_CONTINUE: return parseContinue();

        case TOKEN_IF: return parseIf();

        case TOKEN_WHILE: return parseWhile();
        case TOKEN_FOR: return parseFor();

        case TOKEN_TYPE_INT:
        case TOKEN_TYPE_LONG:
        case TOKEN_TYPE_FLOAT:
        case TOKEN_TYPE_DOUBLE:
        case TOKEN_TYPE_BOOL:
        case TOKEN_TYPE_STRING: return parseDeclaration();

        default: {
            if (isCurrent(TOKEN_IDENTIFIER) && isAssignmentOp(peekNext().type)) return parseAssignment();
            Node *target = parseExpression();
            if (matchCurrent(TOKEN_EQUAL)) {
                if (target->type != NODE_INDEX) exitWithError("Expected an assignable array element");
                IndexNode *index = (IndexNode *)target;
                index->base.type = NODE_INDEX_ASSIGN;
                index->value = parseExpression();
            }
            return target;
        }
    }
}
