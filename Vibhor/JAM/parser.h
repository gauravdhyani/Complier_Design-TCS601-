#ifndef PARSER_H
#define PARSER_H
#define MAX_STATEMENTS 100
#include "lexer.h"
// #include "lexer.c"

typedef enum {
    NODE_TYPE_EXPRESSION,
    // NODE_TYPE_STATEMENT,
    NODE_TYPE_DECLARATION,
    NODE_TYPE_FUNCTION,
    NODE_TYPE_BLOCK,
    // NODE_TYPE_IF,
    // NODE_TYPE_WHILE,
    // NODE_TYPE_FORLOOP,
    NODE_TYPE_RETURN,
    NODE_TYPE_ASSIGNMENT,
    NODE_TYPE_BINARY_OP,
    // NODE_TYPE_UNARY_OP,
    NODE_TYPE_CALL,
    NODE_TYPE_IDENTIFIER,
    NODE_TYPE_LITERAL
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;

    union {
        int num;
        char *str;
        struct {
            struct ASTNode *left;
            Token *op;
            struct ASTNode *right;
        } binary;
        struct {
            struct ASTNode** statements;
            int statement_count;
        } block;
        struct {
            char* name;
            struct ASTNode* value;
        } declaration;
        struct {
            char* name;
            struct ASTNode* body;
        } function;
        struct {
            int num;    // for numbers
            char* str;  // for strings
        } literal;
    } data;
} ASTNode;

typedef struct {
    Token **tokens;
    int current;
    int tokenCount;
} Parser;

/* Parser API */
Parser* init_parser(Token **tokens, int tokenCount);
ASTNode* parse(Parser *parser);
void print_ast(ASTNode *node, int indent);
void free_ast(ASTNode *node);
void print_block(ASTNode *block, int indent);

/* Parsing functions */
ASTNode* parse_declaration(Parser *parser);
ASTNode* parse_function(Parser *parser);
ASTNode* parse_statement(Parser *parser);
ASTNode* parse_expression(Parser *parser);
ASTNode* parse_assignment(Parser *parser);
ASTNode* parse_block(Parser *parser);
ASTNode* parse_declaration(Parser *parser);
ASTNode *parse_program();

/* Utility functions */
Token* parser_peek(Parser *parser);
Token* parser_advance(Parser *parser);
Token* create_token(TokenType type, const char *lexeme, int line, int col);
int parser_match(Parser *parser, TokenType type);
void parser_consume(Parser *parser, TokenType type, const char *error);
void free_ast(ASTNode *node);

#endif