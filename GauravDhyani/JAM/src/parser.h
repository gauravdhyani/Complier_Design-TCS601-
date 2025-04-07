#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION_DECL,
    AST_VAR_DECL,
    AST_RETURN_STMT,
    AST_EXPRESSION,
    AST_BINARY_EXPR,
    AST_LITERAL,
    AST_IDENTIFIER,
    AST_FUNCTION_CALL   
} ASTNodeType;

// AST Node structure
typedef struct ASTNode {
    ASTNodeType type;
    char* value;               // e.g., identifier names, literal values, operator symbols
    struct ASTNode* left;      // left child (or first child in expression, binary, etc.)
    struct ASTNode* right;     // right child (or second child for binary expressions)
    struct ASTNode* next;      // for statement lists (linked list)
} ASTNode;

// Function prototypes for parser functions
ASTNode* parse_program();
ASTNode* parse_statement();
ASTNode* parse_function_decl();
ASTNode* parse_var_decl();
ASTNode* parse_return_stmt();
ASTNode* parse_expression();
ASTNode* parse_term();
ASTNode* parse_factor();

// Utility function to free the AST
void free_ast(ASTNode* node);

#endif
