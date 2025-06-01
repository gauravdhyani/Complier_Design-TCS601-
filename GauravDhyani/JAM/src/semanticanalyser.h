#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "parser.h"


typedef enum {
    TYPE_INT, TYPE_FLOAT, TYPE_BOOL, TYPE_STRING, TYPE_VOID,
    TYPE_ARRAY, TYPE_TUPLE, TYPE_STRUCT, TYPE_FUNCTION, TYPE_UNKNOWN
} TypeKind;

typedef struct Type {
    TypeKind kind;
    union {
        struct { struct Type *elementType; } array;
        struct { struct Type **elements; int count; } tuple;
        struct { char **fieldNames; struct Type **fieldTypes; int count; } structType;
        struct { struct Type **paramTypes; int paramCount; struct Type *returnType; } function;
    };
} Type;

typedef struct SymbolTableEntry {
    char *name;
    Type *type;                       // pointer to Type struct now
    struct SymbolTableEntry *next;   // next symbol in current scope

    // For scope management:
    struct SymbolTableEntry *prevScope;  // linked list of symbols in outer scopes
} SymbolTableEntry;

// Global symbol table
extern SymbolTableEntry *symbolTable;
extern SymbolTableEntry *currentScope;

// Function declarations
void addSymbol(char *name, Type *type);
Type* createFunctionType(Type **paramTypes, int paramCount, Type *returnType);
int typeEquals(Type *a, Type *b);
void enterScope();
TypeKind getNodeType(ASTNode *node);
void exitScope();
SymbolTableEntry *lookupSymbol(const char *name);
int isDeclared(char *name);
Type* getType(ASTNode *node);
int getFunctionArgCount(SymbolTableEntry *entry);
int getASTArgCount(ASTNode *node);
void checkBinaryExpression(ASTNode *node);
void checkVariableDeclaration(ASTNode *node);
void checkVariableUsage(ASTNode *node);
void checkFunctionDeclaration(ASTNode *node);
void checkFunctionCall(ASTNode *node);
void traverse(ASTNode *node);
void performSemanticAnalysis(ASTNode *ast);
void debugTraverse(ASTNode *node);
void checkArrayInitializer(ASTNode *node, Type *expectedType);
void checkWhileLoop(ASTNode *node);
void checkForLoop(ASTNode *node);
#endif // SEMANTIC_ANALYZER_H

