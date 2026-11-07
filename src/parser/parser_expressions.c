#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../utils/cutils.h"
#include "parser.h"
#include "parser_utils.h"
#include "../lexer/lexer.h"
#include "../llvm/lotus_types.h"

static Node *parsePrimary();

static int getPrecedence(TokenType type) {
    switch (type) {
        case TOKEN_OR: return 1;

        case TOKEN_AND: return 2;

        case TOKEN_EQ:
        case TOKEN_NEQ:
        case TOKEN_GT:
        case TOKEN_LT:
        case TOKEN_GE:
        case TOKEN_LE: return 3;

        case TOKEN_PLUS:
        case TOKEN_MINUS: return 4;

        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT: return 5;
        case TOKEN_DOUBLE_STAR: return 6;

        default: return -1;
    }
}

static Node *parseExpressionPrec(const int minPrecedence) {
    Node *left = parsePrimary();

    while (true) {
        TokenType op = current().type;
        int precedence = getPrecedence(op);

        if (precedence < minPrecedence) break;

        advance(); // skip operator

        Node *right = parseExpressionPrec(precedence + 1);
        left = (Node *)newBinaryNode(op, left, right, left->location);
    }

    return left;
}

static Node *parsePrimary() {
    const Token primaryT = current();

    if (isCurrent(TOKEN_MINUS)) {
        if (peekNext().type == TOKEN_NUMBER) {
            advance();
            Token num = advance();
            size_t numLen = strlen(num.value);
            char *newNum = mallocSafe(numLen + 2);
            newNum[0] = '-';
            memcpy(newNum + 1, num.value, numLen);
            newNum[numLen + 1] = '\0';
            Node *node = (Node *)newNumberNode(newNum, primaryT.location);
            free(newNum);
            return node;
        }
        //TODO apply - with any expression
    }
    if (isCurrent(TOKEN_IDENTIFIER) && peekNext().type == TOKEN_LPAREN) {
        Token id = advance();
        IdentifierNode *idN = newIdentifierNode(id.value, LOTUS_FUNCTION, id.location);
        advance(); // skip (

        Node **args = mallocSafe(sizeof(Node *) * MAX_ARGS);
        int argsCount = 0;
        if (!isCurrent(TOKEN_RPAREN)) {
            do {
                if (argsCount >= MAX_ARGS) exitWithError("Too many function arguments");
                args[argsCount++] = parseExpressionPrec(0);
            } while (matchCurrent(TOKEN_COMMA));
        }
        expect(TOKEN_RPAREN);
        return (Node *)newCallNode(idN, argsCount, args, id.location);
    }

    if (matchCurrent(TOKEN_TRUE)) return (Node *)newBooleanNode(true, primaryT.location);
    if (matchCurrent(TOKEN_FALSE)) return (Node *)newBooleanNode(false, primaryT.location);
    if (matchCurrent(TOKEN_STRING)) return (Node *)newStringNode(primaryT.value, primaryT.location);
    if (matchCurrent(TOKEN_NUMBER)) return (Node *)newNumberNode(primaryT.value, primaryT.location);

    if (isCurrent(TOKEN_LPAREN)) {
        advance(); // skip (
        Node *expr = parseExpressionPrec(0);
        expect(TOKEN_RPAREN);
        return expr;
    }

    if (isCurrent(TOKEN_IDENTIFIER)) {
        advance();

        return (Node *)newIdentifierNode(primaryT.value, LOTUS_UNKNOWN, primaryT.location);
    }

    exitWithError("Unexpected token in primary: %d", primaryT.type);
}

Node *parseExpression() {
    return parseExpressionPrec(0);
}
