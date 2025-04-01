#ifndef EXECUTION_ENGINE_H
#define EXECUTION_ENGINE_H

#include "parser.h"

// Structure for symbol table entries
typedef struct SymbolTableEntry {
    char *name;
    char *type;
    int argCount;
    struct SymbolTableEntry *next;
} SymbolTableEntry;

// Structure for environment entries (variables and their values)
typedef struct EnvEntry {
    char *name;
    char *type;
    union {
        int intValue;
        float floatValue;
        char *stringValue;
    } value;
    struct EnvEntry *next;
} EnvEntry;

// Structure for call stack entries (function calls)
typedef struct CallStackEntry {
    char *funcName;
    EnvEntry *env;
    struct CallStackEntry *next;
} CallStackEntry;

// Global symbol table and call stack
extern SymbolTableEntry *symbolTable;
extern CallStackEntry *callStack;

// Function declarations
void freeSymbolTable(SymbolTableEntry *table);
void freeEnv(EnvEntry *env);
void cleanup();
void addSymbol(char *name, char *type, int argCount);
int isDeclared(char *name);
void addEnvEntry(EnvEntry **env, char *name, char *type);
EnvEntry* getEnvEntry(EnvEntry *env, char *name);
void pushCallStack(char *funcName, EnvEntry *env);
void popCallStack();
float evaluateExpression(ASTNode *node, EnvEntry *env);
void executeStatement(ASTNode *node, EnvEntry **env);
void executeFunction(ASTNode *node);
void execute(ASTNode *node);

#endif // EXECUTION_ENGINE_H