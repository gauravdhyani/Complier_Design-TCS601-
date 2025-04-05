#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "semanticanalyser.h"
#include "executionengine.h"

// -------------------------
// Global Structures
// -------------------------
CallStackEntry *callStack = NULL;  // Stack to manage nested function calls

// -------------------------
// Symbol Table Cleanup
// -------------------------

/**
 * Frees all entries in the symbol table.
 * 
 * @param table Pointer to the head of the symbol table.
 * @return void
 */
void freeSymbolTable(SymbolTableEntry *table) {
    while (table) {
        SymbolTableEntry *next = table->next;
        free(table->name);
        free(table->type);
        free(table);
        table = next;
    }
}

// -------------------------
// Cleanup Utilities
// -------------------------

/**
 * Frees all runtime memory including symbol table and call stack.
 *
 * @return void
 */
void cleanup() {
    freeSymbolTable(symbolTable);
    while (callStack) {
        freeEnv(callStack->env);
        popCallStack();
    }
}

// -------------------------
// Environment Management
// -------------------------

/**
 * Retrieves an environment entry for a variable by name.
 * 
 * @param env  Pointer to environment list.
 * @param name Variable name to search for.
 * @return Pointer to EnvEntry if found, NULL otherwise.
 */
EnvEntry* getEnvEntry(EnvEntry *env, char *name) {
    while (env) {
        if (strcmp(env->name, name) == 0) {
            return env;
        }
        env = env->next;
    }
    return NULL;
}

/**
 * Adds a new variable entry to the environment.
 * 
 * @param env  Pointer to pointer of environment list.
 * @param name Name of the variable.
 * @param type Type of the variable (Int, Float, String).
 * @return void
 */
void addEnvEntry(EnvEntry **env, char *name, char *type) {
    EnvEntry *entry = (EnvEntry *)malloc(sizeof(EnvEntry));
    entry->name = strdup(name);
    entry->type = strdup(type);

    if (strcmp(type, "Int") == 0) {
        entry->value.intValue = 0;
    } else if (strcmp(type, "Float") == 0) {
        entry->value.floatValue = 0.0f;
    } else if (strcmp(type, "String") == 0) {
        entry->value.stringValue = NULL;
    } else {
        printf("Error: Unknown type '%s' for variable '%s'.\n", type, name);
        exit(EXIT_FAILURE);
    }

    entry->next = *env;
    *env = entry;
}

/**
 * Frees all entries in the environment list.
 * 
 * @param env Pointer to the environment list.
 * @return void
 */
void freeEnv(EnvEntry *env) {
    while (env) {
        EnvEntry *next = env->next;
        free(env->name);
        free(env->type);
        if (strcmp(env->type, "String") == 0 && env->value.stringValue != NULL) {
            free(env->value.stringValue);
        }
        free(env);
        env = next;
    }
}

// -------------------------
// Call Stack Management
// -------------------------

/**
 * Pushes a function call onto the call stack.
 * 
 * @param funcName Name of the function.
 * @param env      Environment of the function.
 * @return void
 */
void pushCallStack(char *funcName, EnvEntry *env) {
    CallStackEntry *entry = (CallStackEntry *)malloc(sizeof(CallStackEntry));
    entry->funcName = strdup(funcName);
    entry->env = env;
    entry->next = callStack;
    callStack = entry;
}

/**
 * Pops the most recent function call from the call stack.
 * 
 * @return void
 */
void popCallStack() {
    if (callStack) {
        CallStackEntry *entry = callStack;
        callStack = callStack->next;
        free(entry->funcName);
        free(entry);
    }
}

// -------------------------
// Expression Evaluation
// -------------------------

/**
 * Evaluates an AST expression node and returns the resulting value.
 * 
 * @param node AST node representing the expression.
 * @param env  Environment to use for variable lookup.
 * @return Result of expression as float.
 */
float evaluateExpression(ASTNode *node, EnvEntry *env) {
    if (node->type == AST_LITERAL) {
        if (strchr(node->value, '.')) {
            char *endptr;
            float result = strtof(node->value, &endptr);
            if (*endptr != '\0') {
                printf("Error: Invalid float literal '%s'.\n", node->value);
                exit(EXIT_FAILURE);
            }
            return result;
        } else {
            char *endptr;
            int result = strtol(node->value, &endptr, 10);
            if (*endptr != '\0') {
                printf("Error: Invalid integer literal '%s'.\n", node->value);
                exit(EXIT_FAILURE);
            }
            return (float)result;
        }
    } else if (node->type == AST_IDENTIFIER) {
        EnvEntry *entry = getEnvEntry(env, node->value);
        if (!entry) {
            printf("Error: Variable '%s' not found.\n", node->value);
            exit(EXIT_FAILURE);
        }

        if (strcmp(entry->type, "Int") == 0) {
            return (float)entry->value.intValue;
        } else if (strcmp(entry->type, "Float") == 0) {
            return entry->value.floatValue;
        } else {
            printf("Error: Unsupported type '%s' for variable '%s'.\n", entry->type, node->value);
            exit(EXIT_FAILURE);
        }
    } else if (node->type == AST_BINARY_EXPR) {
        float leftValue = evaluateExpression(node->left, env);
        float rightValue = evaluateExpression(node->right, env);

        if (strcmp(node->value, "+") == 0) return leftValue + rightValue;
        if (strcmp(node->value, "-") == 0) return leftValue - rightValue;
        if (strcmp(node->value, "*") == 0) return leftValue * rightValue;
        if (strcmp(node->value, "/") == 0) {
            if (rightValue == 0.0f) {
                printf("Error: Division by zero.\n");
                exit(EXIT_FAILURE);
            }
            return leftValue / rightValue;
        }

        printf("Error: Unsupported binary operator '%s'.\n", node->value);
        exit(EXIT_FAILURE);
    }

    return 0.0f;  // Fallback return
}

// -------------------------
// Statement Execution
// -------------------------

/**
 * Executes a single statement node (e.g., variable declaration or return).
 * 
 * @param node AST node representing the statement.
 * @param env  Pointer to the environment.
 * @return void
 */
void executeStatement(ASTNode *node, EnvEntry **env) {
    if (node->type == AST_VAR_DECL) {
        addEnvEntry(env, node->value, node->left->value);
        float value = evaluateExpression(node->right, *env);

        EnvEntry *entry = getEnvEntry(*env, node->value);
        if (strcmp(entry->type, "Int") == 0) {
            entry->value.intValue = (int)value;
        } else if (strcmp(entry->type, "Float") == 0) {
            entry->value.floatValue = value;
        } else {
            printf("Error: Unsupported type '%s' for variable '%s'.\n", entry->type, node->value);
            exit(EXIT_FAILURE);
        }
    } else if (node->type == AST_RETURN_STMT) {
        float returnValue = evaluateExpression(node->left, *env);
        printf("Return value: %.2f\n", returnValue);
    }
}

// -------------------------
// Function Execution
// -------------------------

/**
 * Executes a function by setting up its environment and running its body.
 * 
 * @param node AST node representing the function definition.
 * @return void
 */
void executeFunction(ASTNode *node) {
    EnvEntry *env = NULL;

    // Setup parameters
    ASTNode *paramNode = node->left->left;
    while (paramNode) {
        addEnvEntry(&env, paramNode->value, paramNode->left->value);
        paramNode = paramNode->next;
    }

    pushCallStack(node->value, env);

    // Execute function body
    ASTNode *stmt = node->left;
    while (stmt) {
        executeStatement(stmt, &env);
        stmt = stmt->next;
    }

    popCallStack();
}

// -------------------------
// Program Execution
// -------------------------

/**
 * Walks through the AST to run the program logic.
 * 
 * @param node AST root node.
 * @return void
 */
void execute(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_LITERAL:
            // Literals don't require action at this stage
            break;
        case AST_EXPRESSION:
            execute(node->left);
            execute(node->right);
            break;
        // Handle other nodes like function declaration if needed
        default:
            break;
    }
}
