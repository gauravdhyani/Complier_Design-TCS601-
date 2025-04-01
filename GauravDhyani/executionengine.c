#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
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
SymbolTableEntry *symbolTable = NULL;
CallStackEntry *callStack = NULL;

// Function to free all the entries in the symbol table
// Input: symbolTable (the symbol table to free)
// Output: None
void freeSymbolTable(SymbolTableEntry *table) {
    while (table) {
        SymbolTableEntry *next = table->next;
        free(table->name);
        free(table->type);
        free(table);
        table = next;
    }
}

// Function to free all the entries in the environment
// Input: env (the environment to free)
// Output: None
void freeEnv(EnvEntry *env) {
    while (env) {
        EnvEntry *next = env->next;
        free(env->name);
        free(env->type);
        if (strcmp(env->type, "String") == 0) {
            free(env->value.stringValue);
        }
        free(env);
        env = next;
    }
}

// Cleanup function to free memory
// Input: None
// Output: None
void cleanup() {
    freeSymbolTable(symbolTable);
    while (callStack) {
        freeEnv(callStack->env);
        popCallStack();
    }
}

// Function to add a symbol (variable or function) to the symbol table
// Input: name (symbol name), type (symbol type), argCount (number of arguments for functions)
// Output: None (adds symbol to symbol table)
void addSymbol(char *name, char *type, int argCount) {
    SymbolTableEntry *entry = (SymbolTableEntry *)malloc(sizeof(SymbolTableEntry));
    entry->name = strdup(name);
    entry->type = strdup(type);
    entry->argCount = argCount;
    entry->next = symbolTable;
    symbolTable = entry;
}

// Function to check if a symbol (variable or function) is declared
// Input: name (symbol name)
// Output: 1 if declared, 0 otherwise
int isDeclared(char *name) {
    SymbolTableEntry *entry = symbolTable;
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return 1;
        }
        entry = entry->next;
    }
    return 0;
}

// Function to add a variable to the environment
// Input: env (pointer to environment), name (variable name), type (variable type)
// Output: None (adds variable to the environment)
void addEnvEntry(EnvEntry **env, char *name, char *type) {
    EnvEntry *entry = (EnvEntry *)malloc(sizeof(EnvEntry));
    entry->name = strdup(name);
    entry->type = strdup(type);
    entry->next = *env;
    *env = entry;
}

// Function to get the value of a variable from the environment
// Input: env (environment), name (variable name)
// Output: pointer to EnvEntry if found, NULL otherwise
EnvEntry* getEnvEntry(EnvEntry *env, char *name) {
    while (env) {
        if (strcmp(env->name, name) == 0) {
            return env;
        }
        env = env->next;
    }
    return NULL;
}

// Function to push a function call onto the call stack
// Input: funcName (function name), env (environment of function arguments)
// Output: None (pushes function call onto call stack)
void pushCallStack(char *funcName, EnvEntry *env) {
    CallStackEntry *entry = (CallStackEntry *)malloc(sizeof(CallStackEntry));
    entry->funcName = strdup(funcName);
    entry->env = env;
    entry->next = callStack;
    callStack = entry;
}

// Function to pop a function call from the call stack
// Input: None
// Output: None (removes function call from the call stack)
void popCallStack() {
    if (callStack) {
        CallStackEntry *entry = callStack;
        callStack = callStack->next;
        free(entry->funcName);
        free(entry);
    }
}

// Function to evaluate an expression
// Input: node (AST node representing the expression), env (environment)
// Output: evaluated float value of the expression
float evaluateExpression(ASTNode *node, EnvEntry *env) {
    if (node->type == AST_LITERAL) {
        // Check if the value is a float or integer
        if (strchr(node->value, '.')) {
            // Attempt to convert to float
            char *endptr;
            float result = strtof(node->value, &endptr);
            if (*endptr != '\0') {
                printf("Error: Invalid float literal '%s'.\n", node->value);
                exit(EXIT_FAILURE);
            }
            return result;  // Handle float literals
        } else {
            // Attempt to convert to int, then cast to float
            char *endptr;
            int result = strtol(node->value, &endptr, 10);
            if (*endptr != '\0') {
                printf("Error: Invalid integer literal '%s'.\n", node->value);
                exit(EXIT_FAILURE);
            }
            return (float)result;  // Handle int literals, cast to float
        }
    } else if (node->type == AST_IDENTIFIER) {
        // Look up the variable in the environment
        EnvEntry *entry = getEnvEntry(env, node->value);
        if (entry) {
            if (strcmp(entry->type, "Int") == 0) {
                return (float)entry->value.intValue;
            } else if (strcmp(entry->type, "Float") == 0) {
                return entry->value.floatValue;
            } else {
                printf("Error: Unsupported type '%s' for variable '%s'.\n", entry->type, node->value);
                exit(EXIT_FAILURE);
            }
        } else {
            printf("Error: Variable '%s' not found.\n", node->value);
            exit(EXIT_FAILURE);
        }
    } else if (node->type == AST_BINARY_EXPR) {
        // Evaluate the left and right operands of the binary expression
        float leftValue = evaluateExpression(node->left, env);
        float rightValue = evaluateExpression(node->right, env);

        // Perform the binary operation
        if (strcmp(node->value, "+") == 0) {
            return leftValue + rightValue;
        } else if (strcmp(node->value, "-") == 0) {
            return leftValue - rightValue;
        } else if (strcmp(node->value, "*") == 0) {
            return leftValue * rightValue;
        } else if (strcmp(node->value, "/") == 0) {
            if (rightValue == 0.0f) {
                printf("Error: Division by zero.\n");
                exit(EXIT_FAILURE);
            }
            return leftValue / rightValue;
        } else {
            printf("Error: Unsupported binary operator '%s'.\n", node->value);
            exit(EXIT_FAILURE);
        }
    }

    // Default case if none of the conditions are met
    return 0.0f;
}

// Function to execute a statement
// Input: node (AST node representing the statement), env (pointer to environment)
// Output: None (executes the statement)
void executeStatement(ASTNode *node, EnvEntry **env) {
    if (node->type == AST_VAR_DECL) {
        // Add variable to the environment
        addEnvEntry(env, node->value, node->left->value);

        // Evaluate the expression for the variable's value
        float value = evaluateExpression(node->right, *env);

        // Look up the variable entry in the environment
        EnvEntry *entry = getEnvEntry(*env, node->value);
        if (strcmp(entry->type, "Int") == 0) {
            // Ensure the value fits in the expected type
            entry->value.intValue = (int)value;
        } else if (strcmp(entry->type, "Float") == 0) {
            // Assign the float value directly
            entry->value.floatValue = value;
        } else {
            printf("Error: Unsupported type '%s' for variable '%s'.\n", entry->type, node->value);
            exit(EXIT_FAILURE);
        }
    } else if (node->type == AST_RETURN_STMT) {
        // Evaluate the return value expression
        float returnValue = evaluateExpression(node->left, *env);

        // Print the return value, formatted based on the type
        printf("Return value: %.2f\n", returnValue);
    }
}

// Function to execute a function
// Input: node (AST node representing the function declaration)
// Output: None (executes the function)
void executeFunction(ASTNode *node) {
    EnvEntry *env = NULL;
    // Add function arguments to the environment
    ASTNode *paramNode = node->left->left; // Assuming parameters are in left->left
    while (paramNode) {
        addEnvEntry(&env, paramNode->value, paramNode->left->value);
        paramNode = paramNode->next;
    }
    pushCallStack(node->value, env);
    ASTNode *stmt = node->left;
    while (stmt) {
        executeStatement(stmt, &env);
        stmt = stmt->next;
    }
    popCallStack();
}

// Function to traverse the AST and execute commands
// Input: node (AST node representing the program)
// Output: None (executes the AST)
void execute(ASTNode *node) {
    if (node->type == AST_FUNCTION_DECL) {
        executeFunction(node);
    } else {
        executeStatement(node, NULL);
    }
    execute(node->left);
    execute(node->right);
    execute(node->next);
}

// Main function: Initializes lexer, parses source code, performs semantic analysis, and executes commands
// Input: None (source code is hardcoded)
// Output: 0 (successful execution)
int main() {
    char* source = 
        "fn main() -> Int {"
        "  var x: Int = 10;"
        "  var y: Int = 20;"
        "  var z: Int = x + y;"
        "  return z;"
        "}";

    initlexer(source);
    ASTNode *parse_tree = parse_program();
    traverse(parse_tree); // Perform semantic analysis
    execute(parse_tree);  // Execute commands

    printf("Execution successful!\n");
    free_ast(parse_tree);
    cleanup(); // Free all allocated memory

    return 0;
}
