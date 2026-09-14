#include <stdio.h>

#include "../parser/parser.h"
#include "utils.h"

static void printNode(const Node *node, const char *prefix, bool isLast);

static const char *typeName(const LotusType type) {
    switch (type) {
        case LOTUS_I32: return "int";
        case LOTUS_I64: return "long";
        case LOTUS_F32: return "float";
        case LOTUS_F64: return "double";
        case LOTUS_BOOL: return "bool";
        case LOTUS_STRING: return "string";
        case LOTUS_VOID: return "void";
        case LOTUS_ANY: return "any";
        case LOTUS_FUNCTION: return "function";
        default: return "unknown";
    }
}

static const char *operatorName(const TokenType op) {
    switch (op) {
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_STAR: return "*";
        case TOKEN_SLASH: return "/";
        case TOKEN_PERCENT: return "%";

        case TOKEN_GT: return ">";
        case TOKEN_LT: return "<";
        case TOKEN_GE: return ">=";
        case TOKEN_LE: return "<=";
        case TOKEN_EQ: return "==";
        case TOKEN_NEQ: return "!=";

        case TOKEN_AND: return "&&";
        case TOKEN_OR: return "||";

        case TOKEN_EQUAL: return "=";

        default: return "?";
    }
}

static void printPrefix(const char *prefix, bool isLast) {
    printf("%s%s", prefix, isLast ? "└── " : "├── ");
}

static void makeChildPrefix(
    const char *prefix,
    bool isLast,
    char *result,
    size_t resultSize
) {
    snprintf(
        result,
        resultSize,
        "%s%s",
        prefix,
        isLast ? "    " : "│   "
    );
}

static void printNode(
    const Node *node,
    const char *prefix,
    bool isLast
) {
    if (!node) {
        printPrefix(prefix, isLast);
        printf("NULL\n");
        return;
    }

    char childPrefix[1024];
    makeChildPrefix(prefix, isLast, childPrefix, sizeof(childPrefix));

    switch (node->type) {
        case NODE_PROGRAM: {
            const ProgramNode *program = (const ProgramNode *) node;

            printPrefix(prefix, isLast);
            printf(
                "Program [%zu %s]\n",
                program->count,
                program->count == 1 ? "statement" : "statements"
            );

            for (size_t i = 0; i < program->count; i++) {
                printNode(
                    program->statements[i],
                    childPrefix,
                    i == program->count - 1
                );
            }

            break;
        }

        case NODE_DECLARATION: {
            const DeclarationNode *declaration =
                    (const DeclarationNode *) node;

            printPrefix(prefix, isLast);
            printf("Declaration : %s\n", typeName(node->dataType));

            printPrefix(childPrefix, false);
            printf("target:\n");

            printNode(
                (Node *) declaration->target,
                childPrefix,
                false
            );

            printPrefix(childPrefix, true);
            printf("value:\n");

            printNode(
                declaration->value,
                childPrefix,
                true
            );

            break;
        }

        case NODE_ASSIGN: {
            const AssignmentNode *assignment =
                    (const AssignmentNode *) node;

            printPrefix(prefix, isLast);
            printf("Assignment\n");

            printPrefix(childPrefix, false);
            printf("target:\n");

            printNode(
                (Node *) assignment->target,
                childPrefix,
                false
            );

            printPrefix(childPrefix, true);
            printf("value:\n");

            printNode(
                assignment->value,
                childPrefix,
                true
            );

            break;
        }

        case NODE_IDENTIFIER: {
            const IdentifierNode *identifier =
                    (const IdentifierNode *) node;

            printPrefix(prefix, isLast);
            printf(
                "Identifier \"%s\" : %s\n",
                identifier->name,
                typeName(node->dataType)
            );

            break;
        }

        case NODE_NUMBER: {
            const NumberNode *number =
                    (const NumberNode *) node;

            printPrefix(prefix, isLast);

            switch (node->dataType) {
                case LOTUS_I32:
                    printf("Number %d : int\n", number->i32);
                    break;

                case LOTUS_I64:
                    printf("Number %lld : long\n", (long long) number->i64);
                    break;

                case LOTUS_F32:
                    printf("Number %g : float\n", number->f32);
                    break;

                case LOTUS_F64:
                    printf("Number %g : double\n", number->f64);
                    break;

                default:
                    printf("Number : %s\n", typeName(node->dataType));
                    break;
            }

            break;
        }

        case NODE_STRING: {
            const StringNode *string =
                    (const StringNode *) node;

            printPrefix(prefix, isLast);
            printf(
                "String \"%s\" : string\n",
                string->value
            );

            break;
        }

        case NODE_BOOLEAN: {
            const BooleanNode *boolean =
                    (const BooleanNode *) node;

            printPrefix(prefix, isLast);
            printf(
                "Bool \"%s\" : bool\n",
                boolean->value == 1 ? "true" : "false"
            );

            break;
        }

        case NODE_BINARY: {
            const BinaryNode *binary =
                    (const BinaryNode *) node;

            printPrefix(prefix, isLast);
            printf(
                "Binary \"%s\" : %s\n",
                operatorName(binary->op),
                typeName(node->dataType)
            );

            printNode(binary->left, childPrefix, false);
            printNode(binary->right, childPrefix, true);

            break;
        }

        case NODE_IF: {
            const IfNode *ifNode = (const IfNode *) node;

            printPrefix(prefix, isLast);
            printf("If\n");

            printPrefix(childPrefix, ifNode->elseBranch != NULL);
            printf("condition:\n");
            printNode(
                ifNode->condition,
                childPrefix,
                true
            );

            printPrefix(childPrefix, ifNode->elseBranch != NULL);
            printf("then:\n");
            printNode(
                ifNode->thenBranch,
                childPrefix,
                ifNode->elseBranch == NULL
            );

            if (ifNode->elseBranch) {
                printPrefix(childPrefix, true);
                printf("else:\n");
                printNode(
                    ifNode->elseBranch,
                    childPrefix,
                    true
                );
            }

            break;
        }

        case NODE_WHILE: {
            const WhileNode *whileNode =
                    (const WhileNode *) node;

            printPrefix(prefix, isLast);
            printf("While\n");

            printPrefix(childPrefix, false);
            printf("condition:\n");
            printNode(
                whileNode->condition,
                childPrefix,
                false
            );

            printPrefix(childPrefix, true);
            printf("body:\n");
            printNode(
                whileNode->body,
                childPrefix,
                true
            );

            break;
        }

        case NODE_CALL: {
            const CallNode *call =
                    (const CallNode *) node;

            printPrefix(prefix, isLast);
            printf(
                "Call \"%s\" [%d %s]\n",
                call->callee->name,
                call->argumentCount,
                call->argumentCount == 1 ? "arg" : "args"
            );

            for (int i = 0; i < call->argumentCount; i++) {
                printNode(
                    call->arguments[i],
                    childPrefix,
                    i == call->argumentCount - 1
                );
            }

            break;
        }

        case NODE_FUNCTION: {
            const FunctionNode *function =
                    (const FunctionNode *) node;

            printPrefix(prefix, isLast);
            printf(
                "Function \"%s\" : %s\n",
                function->name,
                typeName(function->base.dataType)
            );

            if (function->paramCount > 0) {
                printPrefix(childPrefix, false);
                printf("parameters:\n");

                for (int i = 0; i < function->paramCount; i++) {
                    printNode(
                        (Node *) function->params[i],
                        childPrefix,
                        i == function->paramCount - 1
                    );
                }
            }

            printPrefix(childPrefix, true);
            printf("body:\n");

            printNode(
                function->body,
                childPrefix,
                true
            );

            break;
        }

        case NODE_RETURN: {
            const ReturnNode *returnNode =
                    (const ReturnNode *) node;

            printPrefix(prefix, isLast);
            printf("Return\n");

            if (returnNode->value) {
                printNode(
                    returnNode->value,
                    childPrefix,
                    true
                );
            }

            break;
        }

        case NODE_BREAK: {
            const BreakNode *breakNode =
                    (const BreakNode *) node;

            printPrefix(prefix, isLast);
            printf("Break @%d\n", breakNode->level);

            break;
        }

        case NODE_CONTINUE: {
            const ContinueNode *continueNode =
                    (const ContinueNode *) node;

            printPrefix(prefix, isLast);
            printf("Continue @%d\n", continueNode->level);

            break;
        }

        default:
            printPrefix(prefix, isLast);
            printf("Unknown node type: %d\n", node->type);
            break;
    }
}

void printAST(const ProgramNode *node) {
    if (!node) {
        printf("AST: NULL\n");
        return;
    }

    printf("AST\n");
    printNode((Node *) node, "", true);
}
