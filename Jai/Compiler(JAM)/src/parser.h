#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"
typedef enum
{
    NODE_TYPE_EXPRESSION,
    NODE_TYPE_STATEMENT,
    NODE_TYPE_DECLARATION,
    NODE_TYPE_FUNCTION,
    NODE_TYPE_BLOCK,
    NODE_TYPE_IF,
    NODE_TYPE_WHILE,
    NODE_TYPE_FORLOOP,
    NODE_TYPE_RETURN,
    NODE_TYPE_ASSIGNMENT,
    NODE_TYPE_BINARY_OP,
    NODE_TYPE_UNARY_OP,
    NODE_TYPE_CALL,
    NODE_TYPE_IDENTIFIER,
    NODE_TYPE_LITERAL
} ASTNodeType;
typedef struct ASTNode
{
    ASTNodeType type;
    union
    {
        int num;
        char *str;
        struct
        {
            struct ASTNode *left;
            Token *op;
            struct ASTNode *right;
        } binary;
    } data;
} ASTNode;
typedef struct
{
    Token *tokens;
    int current;
    int tokenCount;
} Parser;
