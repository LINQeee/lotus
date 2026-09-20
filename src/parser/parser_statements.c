
#include <stddef.h>
#include <stdlib.h>

#include "../utils/cutils.h"
#include "./parser.h"
#include "./parser_utils.h"
#include "parser_expressions.h"

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
    // if (current().type == TOKEN_LBRACE || current().type == TOKEN_IF) node->value = parseBlock();
    // //TODO return lambda() else node->value = parseExpression(0);
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
    Token typeToken = advance();
    LotusType type = tokenTypeToLotusType(typeToken.type);

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
        TokenType op;
        switch (current().type) {
            case TOKEN_EQ_PLUS: op = TOKEN_PLUS;
                break;
            case TOKEN_EQ_MINUS: op = TOKEN_MINUS;
                break;
            case TOKEN_EQ_STAR: op = TOKEN_STAR;
                break;
            case TOKEN_EQ_SLASH: op = TOKEN_SLASH;
                break;
            case TOKEN_EQ_PERCENT: op = TOKEN_PERCENT;
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

    LotusType type = tokenTypeToLotusType(advance().type);

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

    if (matchCurrent(TOKEN_COLON)) returnType = tokenTypeToLotusType(advance().type);

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

        case TOKEN_TYPE_INT:
        case TOKEN_TYPE_LONG:
        case TOKEN_TYPE_FLOAT:
        case TOKEN_TYPE_DOUBLE:
        case TOKEN_TYPE_BOOL:
        case TOKEN_TYPE_STRING: return parseDeclaration();

        case TOKEN_IDENTIFIER: {
            if (isAssignmentOp(peekNext().type)) return parseAssignment();
            return parseExpression();
        }
        default: return parseExpression();
    }
}
