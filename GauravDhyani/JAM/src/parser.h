#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdlib.h>

typedef enum
{
    AST_NUMBER,
    AST_STRING,
    AST_IDENTIFIER,
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_VAR_DECL,
    AST_RETURN,
    AST_FUNCTION,
    AST_IF,
    AST_PROGRAM,
    AST_TYPE,
    AST_FUNCTION_CALL,
    AST_PRINT_STATEMENT,
    AST_ARRAY_LITERAL,
    AST_WHILE,
    AST_FOR,
    AST_EXPR_STMT,
    // Type categories:
    AST_TYPE_INT,
    AST_TYPE_FLOAT,
    AST_TYPE_BOOL,
    AST_TYPE_STRING,
    AST_TYPE_VOID,
    AST_TYPE_ARRAY,
    AST_TYPE_TUPLE,
    AST_TYPE_STRUCT
} ASTNodeType;
typedef struct ASTNode ASTNode;
typedef struct ASTNode
{
    ASTNodeType type;
    union
    {
        int number;       // AST_NUMBER
        char *string;     // AST_STRING
        char *identifier; // AST_IDENTIFIER

        struct
        { // AST_BINARY_EXPR
            struct ASTNode *left;
            Token *op;
            struct ASTNode *right;
        } binary;

        struct
        { // AST_UNARY_EXPR
            Token *op;
            struct ASTNode *operand;
        } unary;

        struct
        { // AST_VAR_DECL
            char *varName;
            struct ASTNode *varType;     // optional type annotation
            struct ASTNode *initializer; // optional initializer
        } varDecl;

        struct
        { // AST_RETURN
            struct ASTNode *expr;
        } returnStmt;

        struct
        { // AST_FUNCTION
            char *name;
            struct ASTNode **params;
            int paramCount;
            struct ASTNode *returnType;
            struct ASTNode *body;
        } function;

        struct
        { // AST_IF
            struct ASTNode *condition;
            struct ASTNode *thenBranch;
            struct ASTNode *elseBranch;
        } ifStmt;

         struct {
        struct ASTNode** elements;
        int elementCount;
        } arrayLiteral;

        struct {
            struct ASTNode *condition;
            struct ASTNode *body;
        } whileStmt;

        struct {
            struct ASTNode *init;
            struct ASTNode *condition;
            struct ASTNode *increment;
            struct ASTNode *body;
        } forStmt;

        struct
        { // AST_TYPE (for type annotations)
            ASTNodeType typeKind; // AST_TYPE_INT, AST_TYPE_ARRAY, etc.

            union
            {
                struct ASTNode *elementType; // for arrays

                struct
                { // for tuples
                    struct ASTNode **elementTypes;
                    int elementCount;
                } tuple;

                struct
                { // for structs
                    char *name;
                    struct ASTNode **fields; // array of AST_VAR_DECL
                    int fieldCount;
                } structType;
            };
        } type;

        struct {
            ASTNode *expr;
        } ExprStmt;

        struct
        { // AST_FUNCTION_CALL
            struct ASTNode *callee;
            struct ASTNode **arguments;
            int argCount;
        } call;

        struct
        { // AST_PRINT_STATEMENT
            struct ASTNode *expr;
        } printStmt;

        struct
        { // AST_PROGRAM
            struct ASTNode **statements;
            int count;
        } program;

    } data;
} ASTNode;

typedef struct
{
    Token **tokens;
    int current;
    int tokenCount;
} Parser;

void initParser(Parser *p, Token **tokens, int tokenCount);
ASTNode *parseProgram(Parser *p);
void printAST(ASTNode *node, int indent);
void freeAST(ASTNode *node);

#endif // PARSER_H
 