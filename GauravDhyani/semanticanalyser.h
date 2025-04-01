#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "parser.h"

// Structure for symbol table entries
typedef struct SymbolTableEntry {
    char *name;
    char *type;
    int argCount;
    struct SymbolTableEntry *next;
} SymbolTableEntry;

// Global symbol table
extern SymbolTableEntry *symbolTable;

// Function declarations
void addSymbol(char *name, char *type, int argCount);
int isDeclared(char *name);
char* getType(ASTNode *node);
int getFunctionArgCount(SymbolTableEntry *entry);
int getASTArgCount(ASTNode *node);
void checkBinaryExpression(ASTNode *node);
void checkVariableDeclaration(ASTNode *node);
void checkVariableUsage(ASTNode *node);
void checkFunctionDeclaration(ASTNode *node);
void checkFunctionCall(ASTNode *node);
void traverse(ASTNode *node);

#endif // SEMANTIC_ANALYZER_H