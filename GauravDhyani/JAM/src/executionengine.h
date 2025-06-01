#ifndef EXECUTION_ENGINE_H
#define EXECUTION_ENGINE_H

#include "parser.h"
#include "semanticanalyser.h"
#include <stdbool.h>

typedef enum {
    VALUE_INT,
    VALUE_FLOAT,
    VALUE_BOOL,
    VALUE_STRING,
    VALUE_ARRAY
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        int intValue;
        float floatValue;
        int boolValue;
        char *stringValue;
        struct {
            struct Value **elements;
            int count;
        } arrayValue;
    };
} Value;
// Structure for environment entries (variables and their values)
typedef struct EnvEntry {
    char *name;
    ASTNode *typeAnnotation;  
    Value *storedValue;
    union {
        int intValue;
        float floatValue;
        int boolValue;
        char *stringValue;
    } value;
    struct ASTNode *functionNode;
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

void freeType(Type *type);
void freeSymbolTable(SymbolTableEntry *table);
EnvEntry* getEnvEntry(EnvEntry *env, char *name);
void addEnvEntry(EnvEntry **env, char *name, ASTNode *typeAnnotation);
void freeEnv(EnvEntry *env);
void pushCallStack(char *funcName, EnvEntry *env);
void popCallStack(void);
Value* evaluateExpression(ASTNode *node, EnvEntry *env);
void executeStatement(ASTNode *node, EnvEntry **env, float *outReturnValue, bool *outHasReturned);
float executeFunction(ASTNode *funcNode, ASTNode **args, int argCount);
void execute(ASTNode *node);
int run_jam_script(const char *filename);
void dumpEnvEntries(EnvEntry *env);

#ifdef __cplusplus
}
#endif

// Leave these outside if defined in C++
void addSymbol(char *name, Type *type);
int isDeclared(char *name);

#endif // EXECUTION_ENGINE_H


