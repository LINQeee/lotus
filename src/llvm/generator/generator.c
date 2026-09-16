
#include <stdbool.h>
#include <stdlib.h>

#include "../../parser/parser.h"
#include "expression/expression_generator.h"
#include "statement/statement_generator.h"

static bool isStatement(const Node *node) {
    return node->type == NODE_ASSIGN || node->type == NODE_DECLARATION || node->type == NODE_PROGRAM ||
           node->type == NODE_WHILE || node->type == NODE_BREAK || node->type == NODE_CONTINUE ||
           node->type == NODE_RETURN || node->type == NODE_IF || node->type == NODE_FUNCTION;
}

void generateNode(const Node *node) {
    if (isStatement(node)) generateStatement(node);
    else generateExpression(node);
}
