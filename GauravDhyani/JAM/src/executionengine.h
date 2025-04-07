#ifndef EXECUTION_ENGINE_H
#define EXECUTION_ENGINE_H

#include "parser.h"
#include "semanticanalyser.h"

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

// C++ linkage-aware section
#ifdef __cplusplus
extern "C" {
#endif

void freeSymbolTable(SymbolTableEntry *table);
void freeEnv(EnvEntry *env);
void cleanup();
void addEnvEntry(EnvEntry **env, char *name, char *type);
EnvEntry* getEnvEntry(EnvEntry *env, char *name);
void pushCallStack(char *funcName, EnvEntry *env);
void popCallStack();
float evaluateExpression(ASTNode *node, EnvEntry *env);
void executeStatement(ASTNode *node, EnvEntry **env);
void executeFunction(ASTNode *node);
void execute(ASTNode *node);
int run_jam_script(const char *filename);

#ifdef __cplusplus
}
#endif

// Leave these outside if defined in C++
void addSymbol(char *name, char *type, int argCount);
int isDeclared(char *name);

#endif // EXECUTION_ENGINE_H

