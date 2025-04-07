#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "semanticanalyser.h"

// Global symbol table head
SymbolTableEntry *symbolTable = NULL;

/**
 * @brief Adds a symbol (variable or function) to the symbol table.
 *
 * @param name The name of the symbol.
 * @param type The type of the symbol (e.g., int, float, function return type).
 * @param argCount The number of arguments (used for functions).
 */
void addSymbol(char *name, char *type, int argCount) {
    SymbolTableEntry *entry = (SymbolTableEntry *)malloc(sizeof(SymbolTableEntry));
    entry->name = strdup(name);
    entry->type = strdup(type);
    entry->argCount = argCount;
    entry->next = symbolTable;
    symbolTable = entry;
}

/**
 * @brief Checks whether a symbol (variable or function) has been declared.
 *
 * @param name The name of the symbol.
 * @return 1 if declared, 0 otherwise.
 */
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

/**
 * @brief Returns the type of a given AST node.
 *
 * @param node The AST node (identifier or literal).
 * @return The type or value string if applicable, or "unknown".
 */
char* getType(ASTNode *node) {
    if (node->type == AST_IDENTIFIER || node->type == AST_LITERAL) {
        return node->value;
    }
    return "unknown";
}

/**
 * @brief Retrieves the number of arguments associated with a function symbol.
 *
 * @param entry The symbol table entry for the function.
 * @return The number of expected arguments.
 */
int getFunctionArgCount(SymbolTableEntry *entry) {
    return entry->argCount;
}

/**
 * @brief Counts the number of arguments passed to a function in the AST.
 *
 * @param node The function call node in the AST.
 * @return Number of argument nodes.
 */
int getASTArgCount(ASTNode *node) {
    int count = 0;
    ASTNode *arg = node->left;
    while (arg) {
        count++;
        arg = arg->next;
    }
    return count;
}

/**
 * @brief Checks for type mismatches in binary expressions.
 *
 * @param node The AST node representing a binary expression.
 */
void checkBinaryExpression(ASTNode *node) {
    char *leftType = getType(node->left);
    char *rightType = getType(node->right);

    if (strcmp(leftType, rightType) != 0) {
        printf("Type Error: Mismatched types '%s' and '%s' in expression.\n", leftType, rightType);
    }
}

/**
 * @brief Checks for redeclaration of variables and adds them to the symbol table.
 *
 * @param node The AST node representing a variable declaration.
 */
void checkVariableDeclaration(ASTNode *node) {
    if (isDeclared(node->value)) {
        printf("Error: Variable '%s' already declared.\n", node->value);
    } else {
        addSymbol(node->value, node->left->value, 0);
    }
}

/**
 * @brief Ensures a variable is declared before its usage.
 *
 * @param node The AST node representing an identifier.
 */
void checkVariableUsage(ASTNode *node) {
    if (!isDeclared(node->value)) {
        printf("Error: Variable '%s' used before declaration.\n", node->value);
    }
}

/**
 * @brief Checks for function redeclaration and adds them to the symbol table.
 *
 * @param node The AST node representing a function declaration.
 */
void checkFunctionDeclaration(ASTNode *node) {
    if (isDeclared(node->value)) {
        printf("Error: Function '%s' already declared.\n", node->value);
    } else {
        addSymbol(node->value, node->left->value, getASTArgCount(node->left));
    }
}

/**
 * @brief Validates function calls for declaration and argument count.
 *
 * @param node The AST node representing a function call.
 */
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

/**
 * @brief Traverses the AST recursively and performs semantic analysis checks.
 *
 * @param node The current AST node being visited.
 */
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
        case AST_PROGRAM:
        case AST_RETURN_STMT:
        case AST_EXPRESSION:
        case AST_LITERAL:
            break;
    }

    traverse(node->left);
    traverse(node->right);
    traverse(node->next);
}

/**
 * Example main for testing semantic analysis (commented).
 */
/*
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
*/
