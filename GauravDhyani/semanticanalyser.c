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

// Global symbol table
SymbolTableEntry *symbolTable = NULL;

// Adds a symbol (variable or function) to the symbol table
// Input: name (symbol name), type (data type or return type), argCount (function argument count)
// Output: None
void addSymbol(char *name, char *type, int argCount) {
    SymbolTableEntry *entry = (SymbolTableEntry *)malloc(sizeof(SymbolTableEntry));
    entry->name = strdup(name);
    entry->type = strdup(type);
    entry->argCount = argCount;
    entry->next = symbolTable;
    symbolTable = entry;
}

// Checks if a symbol (variable or function) is declared
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

// Retrieves the type of an AST node (identifier or literal)
// Input: node (AST node)
// Output: Type of the node as a string
char* getType(ASTNode *node) {
    if (node->type == AST_IDENTIFIER || node->type == AST_LITERAL) {
        return node->value;
    }
    return "unknown";
}

// Retrieves argument count for a function from the symbol table
// Input: entry (symbol table entry for the function)
// Output: Number of arguments
int getFunctionArgCount(SymbolTableEntry *entry) {
    return entry->argCount;
}

// Counts the number of arguments in an AST function call node
// Input: node (AST function call node)
// Output: Number of arguments in the function call
int getASTArgCount(ASTNode *node) {
    int count = 0;
    ASTNode *arg = node->left;
    while (arg) {
        count++;
        arg = arg->next;
    }
    return count;
}

// Checks for type mismatches in a binary expression
// Input: node (AST binary expression node)
// Output: Prints an error message if types do not match
void checkBinaryExpression(ASTNode *node) {
    char *leftType = getType(node->left);
    char *rightType = getType(node->right);

    if (strcmp(leftType, rightType) != 0) {
        printf("Type Error: Mismatched types '%s' and '%s' in expression.\n", leftType, rightType);
    }
}

// Checks for variable redeclaration and adds it to the symbol table if valid
// Input: node (AST variable declaration node)
// Output: Prints an error if the variable is already declared
void checkVariableDeclaration(ASTNode *node) {
    if (isDeclared(node->value)) {
        printf("Error: Variable '%s' already declared.\n", node->value);
    } else {
        addSymbol(node->value, node->left->value, 0);
    }
}

// Checks if a variable is used before declaration
// Input: node (AST identifier node)
// Output: Prints an error if the variable is undeclared
void checkVariableUsage(ASTNode *node) {
    if (!isDeclared(node->value)) {
        printf("Error: Variable '%s' used before declaration.\n", node->value);
    }
}

// Checks for function redeclaration and adds it to the symbol table if valid
// Input: node (AST function declaration node)
// Output: Prints an error if the function is already declared
void checkFunctionDeclaration(ASTNode *node) {
    if (isDeclared(node->value)) {
        printf("Error: Function '%s' already declared.\n", node->value);
    } else {
        addSymbol(node->value, node->left->value, getASTArgCount(node->left));
    }
}

// Checks if a function is declared before use and verifies argument count
// Input: node (AST function call node)
// Output: Prints an error if function is undeclared or argument count mismatch
void checkFunctionCall(ASTNode *node) {
    if (!isDeclared(node->value)) {
        printf("Error: Function '%s' not declared.\n", node->value);
        return;
    }

    SymbolTableEntry *funcEntry = symbolTable;
    while (funcEntry && strcmp(funcEntry->name, node->value) != 0) {
        funcEntry = funcEntry->next;
    }

    if (funcEntry) {
        int expectedArgs = getFunctionArgCount(funcEntry);
        int actualArgs = getASTArgCount(node);

        if (expectedArgs != actualArgs) {
            printf("Error: Function '%s' expected %d arguments but got %d.\n", node->value, expectedArgs, actualArgs);
        }
    }
}

// Traverses the AST and performs semantic checks
// Input: node (AST root node)
// Output: Performs checks and prints error messages if violations are found
void traverse(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_VAR_DECL:
            checkVariableDeclaration(node);
            break;
        case AST_FUNCTION_DECL:
            checkFunctionDeclaration(node);
            break;
        case AST_IDENTIFIER:
            checkVariableUsage(node);
            break;
        case AST_FUNCTION_CALL:
            checkFunctionCall(node);
            break;
        case AST_BINARY_EXPR:
            checkBinaryExpression(node);
            break;
    }

    traverse(node->left);
    traverse(node->right);
    traverse(node->next);
}

// Main function: Initializes lexer, parses source code, performs semantic analysis
// Input: None (uses a hardcoded test source string)
// Output: Prints semantic analysis errors or success message
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
    traverse(parse_tree);

    printf("Semantic analysis successful!\n");
    free_ast(parse_tree);

    return 0;
}
