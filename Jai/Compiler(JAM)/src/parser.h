#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdlib.h>

typedef enum {
    AST_NUMBER,
    AST_STRING,
    AST_IDENTIFIER,
    AST_BINARY_EXPR,
    AST_VAR_DECL,
    AST_RETURN,
    AST_FUNCTION,  // <-- new
    AST_PROGRAM
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    union {
        int               number;   // AST_NUMBER
        char*             string;   // AST_STRING
        char*             identifier; // AST_IDENTIFIER
        struct {                    // AST_BINARY_EXPR
            struct ASTNode* left;
            Token*           operator;
            struct ASTNode* right;
        } binary;
        struct {                    // AST_VAR_DECL
            char*            varName;
            struct ASTNode* initializer;  // may be NULL
        } varDecl;
        struct {                    // AST_RETURN
            struct ASTNode* expr;
        } returnStmt;
        struct {                    // AST_FUNCTION
            char*            name;
            struct ASTNode* body;       // a BLOCK represented as AST_PROGRAM
        } function;
        struct {                    // AST_PROGRAM (top-level)
            struct ASTNode** statements;
            int               count;
        } program;
    } data;
} ASTNode;

typedef struct {
    Token** tokens;
    int     current;
    int     tokenCount;
} Parser;

void initParser(Parser* p, Token** tokens, int tokenCount);
ASTNode* parseProgram(Parser* p);
void printAST(ASTNode* node, int indent);
void freeAST(ASTNode* node);

#endif // PARSER_H
