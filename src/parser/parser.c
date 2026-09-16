#include "parser.h"
#include "../lexer/lexer.h"
#include "parser_utils.h"

#include "../utils/cutils.h"
#include "./parser_statements.h"

static const Token *tokens;
static size_t pos = 0;

Token current() {
    return tokens[pos];
}
Token advance() {
    return tokens[pos++];
}
Token peekNext() {
    return tokens[pos + 1];
}
bool isCurrent(TokenType type) {
    return current().type == type;
}
bool matchCurrent(TokenType type) {
    if (!isCurrent(type)) return false;

    advance();
    return true;
}
Token expect(TokenType type) {
    if (!isCurrent(type)) exitWithError("Expected %d but got %d (%s)", type, current().type, current().value);
    return advance();
}

static ProgramNode *parseProgram() {
    ProgramNode *program = newProgramNode();
    while (!isCurrent(TOKEN_EOF)) programAddStatement(program, parseStatement());
    return program;
}

ProgramNode *buildAST(const Token *tokensToParse) {
    pos = 0;
    tokens = tokensToParse;
    ProgramNode *program = parseProgram();
    return program;
}
