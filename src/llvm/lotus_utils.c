#include "../lexer/lexer.h"
#include "./lotus_types.h"

#include <stdbool.h>

const char *lotusTypeName(LotusType type) {
  switch (type) {
  case LOTUS_I32:
    return "int";

  case LOTUS_I64:
    return "long";

  case LOTUS_F32:
    return "float";

  case LOTUS_F64:
    return "double";

  case LOTUS_BOOL:
    return "bool";

  case LOTUS_STRING:
    return "string";

  case LOTUS_VOID:
    return "void";

  case LOTUS_ANY:
    return "any";

  case LOTUS_FUNCTION:
    return "function";
  }

  return "<unknown>";
}

const char *tokenTypeName(TokenType type) {
  switch (type) {
  case TOKEN_PLUS:
    return "+";
  case TOKEN_MINUS:
    return "-";
  case TOKEN_STAR:
    return "*";
  case TOKEN_SLASH:
    return "/";
  case TOKEN_PERCENT:
    return "%";

  case TOKEN_EQ:
    return "==";
  case TOKEN_NEQ:
    return "!=";
  case TOKEN_GT:
    return ">";
  case TOKEN_LT:
    return "<";
  case TOKEN_GE:
    return ">=";
  case TOKEN_LE:
    return "<=";

  case TOKEN_AND:
    return "&&";
  case TOKEN_OR:
    return "||";

  case TOKEN_EQUAL:
    return "=";

  default:
    return "<operator>";
  }
}

bool isNumericType(LotusType type) {
  return type == LOTUS_I32 || type == LOTUS_I64 || type == LOTUS_F32 ||
         type == LOTUS_F64;
}

bool isIntegerType(LotusType type) {
  return type == LOTUS_I32 || type == LOTUS_I64;
}

bool isBooleanType(LotusType type) { return type == LOTUS_BOOL; }
