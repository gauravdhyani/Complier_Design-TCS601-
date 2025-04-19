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
    if (node->type == NODE_TYPE_IDENTIFIER) {
        return "identifier";  // Or fetch type from symbol table
    } else if (node->type == NODE_TYPE_LITERAL) {
        // Check if numeric or string literal
        return (node->data.literal.str) ? "string" : "int";
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
    if (node->type == NODE_TYPE_CALL) {
        // Example: Assume arguments are stored in a block
        count = node->data.block.statement_count;
    }
    return count;
}

/**
 * @brief Checks for type mismatches in binary expressions.
 *
 * @param node The AST node representing a binary expression.
 */
void checkBinaryExpression(ASTNode *node) {
    // char *leftType = getType(node->left);
    // char *rightType = getType(node->right);

    char *leftType = getType(node->data.binary.left);
    char *rightType = getType(node->data.binary.right);

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
    char *var_name = node->data.declaration.name;
    if (isDeclared(var_name)){
        printf("Error: Variable '%s' already declared.\n", var_name);
    } 
    char *var_type = getType(node->data.declaration.value);
    addSymbol(var_name, var_type, 0);
}

/**
 * @brief Ensures a variable is declared before its usage.
 *
 * @param node The AST node representing an identifier.
 */

void checkVariableUsage(ASTNode *node) {
    //  Identifier name is now stored in `data.str`
    char *var_name = node->data.str; 

    if (!isDeclared(var_name)) {
        printf("Error: Variable '%s' used before declaration.\n", var_name);
    }
}

/**
 * @brief Checks for function redeclaration and adds them to the symbol table.
 *
 * @param node The AST node representing a function declaration.
 */

void checkFunctionDeclaration(ASTNode *node) {
    char *func_name = node->data.function.name;
    if (isDeclared(func_name)) { 
        printf("Error: Function '%s' already declared.\n", func_name);
    }
    // Assuming return type is stored elsewhere (adjust as needed)
    addSymbol(func_name, "function", 0 /* Update arg count logic */);
}

/**
 * @brief Validates function calls for declaration and argument count.
 *
 * @param node The AST node representing a function call.
 */
void checkFunctionCall(ASTNode *node) {
    char *func_name = node->data.function.name;
    if (!isDeclared(func_name)) {
        printf("Error: Function '%s' not declared.\n", func_name);
        return;
    }

    SymbolTableEntry *funcEntry = symbolTable;
    while (funcEntry && strcmp(funcEntry->name, func_name) != 0) {
        funcEntry = funcEntry->next;
    }

    if (funcEntry) {
        int expectedArgs = getFunctionArgCount(funcEntry);
        // int actualArgs = getASTArgCount(node);
        int actualArgs = node->data.block.statement_count;

        if (expectedArgs != actualArgs) {
            printf("Error: Function '%s' expected %d arguments but got %d.\n", func_name, expectedArgs, actualArgs);
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
        case NODE_TYPE_DECLARATION:
            checkVariableDeclaration(node);
            break;
        case NODE_TYPE_FUNCTION:
            checkFunctionDeclaration(node);
            break;
        case NODE_TYPE_IDENTIFIER:
            checkVariableUsage(node);
            break;
        case NODE_TYPE_CALL:
            checkFunctionCall(node);
            break;
        case NODE_TYPE_BINARY_OP:
            checkBinaryExpression(node);
            break;
        // case AST_PROGRAM:
        case NODE_TYPE_RETURN:
        case NODE_TYPE_EXPRESSION:
        case NODE_TYPE_LITERAL:
        case NODE_TYPE_BLOCK:
        case NODE_TYPE_ASSIGNMENT:
            break;
    }

    if (node->type == NODE_TYPE_BLOCK) {
        for (int i = 0; i < node->data.block.statement_count; i++) {
            traverse(node->data.block.statements[i]);
        }
    } else {
        // Traverse other nodes (e.g., binary ops)
        if (node->type == NODE_TYPE_BINARY_OP) {
            traverse(node->data.binary.left);
            traverse(node->data.binary.right);
        }
        // Add more cases as needed
    }
}

/**
 * Example main for testing semantic analysis (commented).
 */

// int main() {
//     // char* source = 
//     //     "fn main() -> Int {"
//     //     "  var x: Int = 10;"
//     //     "  var y: Int = 20;"
//     //     "  var z: Int = x + y;"
//     //     "  return z;"
//     //     "}";

//     char* source = "fn main() { var x = 10; return x + 5; }";

//     initlexer(source);
//     ASTNode *parse_tree = parse_program();
//     traverse(parse_tree);

//     printf("Semantic analysis successful!\n");
//     // free_ast(parse_tree);

//     return 0;
// }

