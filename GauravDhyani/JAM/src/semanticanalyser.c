#include "semanticanalyser.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global symbol table head
SymbolTableEntry *symbolTable = NULL;
SymbolTableEntry *currentScope = NULL;

/**
 * @brief Adds a symbol (variable or function) to the symbol table.
 *
 * @param name The name of the symbol.
 * @param type The type of the symbol (e.g., int, float, function return type).
 * @param argCount The number of arguments (used for functions).
 */
void addSymbol(char *name, Type *type)
{
    SymbolTableEntry *entry = malloc(sizeof(SymbolTableEntry));
    entry->name = strdup(name);
    entry->type = type;
    entry->next = currentScope;
    entry->prevScope = NULL; // For nested scopes, see enterScope

    currentScope = entry;
}

/**
 * @brief Creates a function type object with specified parameter types and return type.
 *
 * @param paramTypes Array of pointers to parameter types.
 * @param paramCount Number of parameters.
 * @param returnType Pointer to the return type.
 * @return Pointer to the newly created function Type object.
 */
Type *createFunctionType(Type **paramTypes, int paramCount, Type *returnType)
{
    Type *funcType = malloc(sizeof(Type));
    funcType->kind = TYPE_FUNCTION;
    funcType->function.paramTypes = paramTypes;
    funcType->function.paramCount = paramCount;
    funcType->function.returnType = returnType;
    return funcType;
}

/**
 * @brief Compares two types for equality, recursively checking complex types.
 *
 * @param a Pointer to the first Type.
 * @param b Pointer to the second Type.
 * @return 1 if types are equal, 0 otherwise.
 */
int typeEquals(Type *a, Type *b)
{
    if (a == NULL || b == NULL)
        return 0;
    if (a->kind != b->kind)
        return 0;
    // For complex types, compare recursively
    switch (a->kind)
    {
    case TYPE_ARRAY:
        return typeEquals(a->array.elementType, b->array.elementType);
    case TYPE_FUNCTION:
        if (a->function.paramCount != b->function.paramCount)
            return 0;
        for (int i = 0; i < a->function.paramCount; i++)
        {
            if (!typeEquals(a->function.paramTypes[i], b->function.paramTypes[i]))
                return 0;
        }
        return typeEquals(a->function.returnType, b->function.returnType);
    // similarly for other complex types
    default:
        return 1;
    }
}

/**
 * @brief Enters a new scope by pushing the current scope chain to a previous scope
 *        and resetting the current scope to empty.
 */
void enterScope()
{
    SymbolTableEntry *newScope = NULL;
    // Push current scope chain to prevScope for all entries in currentScope
    for (SymbolTableEntry *entry = currentScope; entry != NULL; entry = entry->next)
    {
        entry->prevScope = newScope;
        newScope = entry;
    }
    currentScope = NULL; // new empty scope
}

/**
 * @brief Exits the current scope by freeing all symbol table entries in the current scope
 *        and restoring the previous outer scope.
 *
 * Note: This implementation sets currentScope to NULL after freeing;
 *       adjusting to restore outer scope depends on symbol table design.
 */
void exitScope()
{
    // Free symbols in current scope
    SymbolTableEntry *entry = currentScope;
    while (entry)
    {
        SymbolTableEntry *next = entry->next;
        free(entry->name);
        // Ideally also free type recursively here
        free(entry);
        entry = next;
    }
    // Restore outer scope
    currentScope = NULL; // or set to outer scope saved in prevScope, depends on implementation
}

/**
 * @brief Looks up a symbol by name in the current scope.
 *
 * @param name The symbol name to look for.
 * @return Pointer to the SymbolTableEntry if found, or NULL if not found.
 */
SymbolTableEntry *lookupSymbol(const char *name)
{
    SymbolTableEntry *entry = currentScope;
    while (entry)
    {
        if (strcmp(entry->name, name) == 0)
        {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/**
 * @brief Checks if a symbol with the given name is declared in the current scope.
 *
 * @param name The symbol name to check.
 * @return Non-zero if declared, zero otherwise.
 */
int isDeclared(char *name)
{
    return lookupSymbol(name) != NULL;
}

/**
 * @brief Returns the type of a given AST node.
 *
 * @param node The AST node (identifier, literal, or expression).
 * @return The type string if determined (e.g., "int"), "Undeclared" if an identifier isn't in the symbol table,
 *         "TypeError" if binary operand types mismatch, or "Unknown" if the node is null or unrecognized.
 */
Type *getType(ASTNode *node) {
    if (!node)
        return NULL;

    switch (node->type) {
        case AST_NUMBER: {
            static Type intType = {.kind = TYPE_INT};
            return &intType;
        }

        case AST_STRING: {
            static Type stringType = {.kind = TYPE_STRING};
            return &stringType;
        }

        case AST_IDENTIFIER: {
            SymbolTableEntry *entry = lookupSymbol(node->data.identifier);
            return entry ? entry->type : NULL;
        }

        case AST_BINARY_EXPR: {
            Type *leftType = getType(node->data.binary.left);
            Type *rightType = getType(node->data.binary.right);
            if (leftType && rightType && leftType->kind == rightType->kind) {
                return leftType;
            }
            return NULL; // type mismatch
        }

        case AST_TYPE: {
            ASTNodeType kind = node->data.type.typeKind;
            switch (kind) {
                case AST_TYPE_INT: {
                    static Type intType = {.kind = TYPE_INT};
                    return &intType;
                }
                case AST_TYPE_FLOAT: {
                    static Type floatType = {.kind = TYPE_FLOAT};
                    return &floatType;
                }
                case AST_TYPE_BOOL: {
                    static Type boolType = {.kind = TYPE_BOOL};
                    return &boolType;
                }
                case AST_TYPE_STRING: {
                    static Type stringType = {.kind = TYPE_STRING};
                    return &stringType;
                }
                case AST_TYPE_VOID: {
                    static Type voidType = {.kind = TYPE_VOID};
                    return &voidType;
                }
                case AST_TYPE_ARRAY: {
                    Type *elementType = getType(node->data.type.elementType);
                    if (!elementType) return NULL;
                    Type *arrayType = malloc(sizeof(Type));
                    arrayType->kind = TYPE_ARRAY;
                    arrayType->array.elementType = elementType;
                    return arrayType;
                }
                case AST_TYPE_TUPLE: {
                    int count = node->data.type.tuple.elementCount;
                    Type **elements = malloc(sizeof(Type *) * count);
                    for (int i = 0; i < count; i++) {
                        elements[i] = getType(node->data.type.tuple.elementTypes[i]);
                        if (!elements[i]) return NULL;
                    }
                    Type *tupleType = malloc(sizeof(Type));
                    tupleType->kind = TYPE_TUPLE;
                    tupleType->tuple.elements = elements;
                    tupleType->tuple.count = count;
                    return tupleType;
                }
                case AST_TYPE_STRUCT: {
                    int count = node->data.type.structType.fieldCount;
                    char **fieldNames = malloc(sizeof(char *) * count);
                    Type **fieldTypes = malloc(sizeof(Type *) * count);
                    for (int i = 0; i < count; i++) {
                        ASTNode *field = node->data.type.structType.fields[i];
                        fieldNames[i] = field->data.varDecl.varName;
                        fieldTypes[i] = getType(field->data.varDecl.varType);
                        if (!fieldTypes[i]) return NULL;
                    }
                    Type *structType = malloc(sizeof(Type));
                    structType->kind = TYPE_STRUCT;
                    structType->structType.fieldNames = fieldNames;
                    structType->structType.fieldTypes = fieldTypes;
                    structType->structType.count = count;
                    return structType;
                }
                default:
                    return NULL;
            }
        }

        default:
            return NULL;
    }
}

/**
 * @brief Retrieves the number of arguments associated with a function symbol.
 *
 * @param entry The symbol table entry for the function.
 * @return The number of expected arguments, or -1 if the entry is NULL.
 */
int getFunctionArgCount(SymbolTableEntry *entry)
{
    if (!entry || !entry->type)
        return -1;
    if (entry->type->kind != TYPE_FUNCTION)
        return -1;
    return entry->type->function.paramCount;
}

/**
 * @brief Recursively counts the number of argument nodes within an AST program node.
 *
 * @param node The AST program node representing a list of function arguments.
 * @return The total number of argument nodes found.
 */
int getASTArgCount(ASTNode *node)
{
    if (!node)
        return 0;
    if (node->type != AST_PROGRAM)
        return 1;

    int count = 0;
    for (int i = 0; i < node->data.program.count; i++)
    {
        count += getASTArgCount(node->data.program.statements[i]);
    }
    return count;
}

/**
 * @brief Checks for type mismatches in a binary expression node.
 *
 * @param node The AST node representing a binary expression.
 *             Assumes the node is non-null and of type AST_BINARY_EXPR.
 */
void checkBinaryExpression(ASTNode *node)
{
    Type *leftType = getType(node->data.binary.left);
    Type *rightType = getType(node->data.binary.right);
    if (!typeEquals(leftType, rightType))
    {
        printf("Semantic Error: Type mismatch in binary expression\n");
    }
}

/**
 * @brief Checks for redeclaration of a variable and adds it to the symbol table if not already declared.
 *
 * @param node The AST node representing a variable declaration.
 *             Assumes the node is non-null and of type AST_VARIABLE_DECL.
 */
void checkVariableDeclaration(ASTNode *node)
{
    char *varName = node->data.varDecl.varName;
    if (isDeclared(varName))
    {
        printf("Semantic Error: Variable '%s' already declared.\n", varName);
    }
    else
    {
        Type *varType = getType(node->data.varDecl.varType);
        addSymbol(varName, varType);
    }
}

/**
 * @brief Ensures a variable is declared before its usage.
 *
 * @param node The AST node representing an identifier.
 *             Assumes the node is non-null and of type AST_IDENTIFIER.
 */
void checkVariableUsage(ASTNode *node)
{
    char *varName = node->data.identifier;
    if (!isDeclared(varName))
    {
        printf("Semantic Error: Variable '%s' used before declaration.\n", varName);
    }
}

/**
 * @brief Checks for function redeclaration and, if not already declared,
 *        adds the function symbol with its complete type information
 *        (parameter types and return type) to the symbol table.
 *
 * @param node The AST node representing a function declaration.
 *             Assumes the node is non-null and of type AST_FUNCTION.
 */
void checkFunctionDeclaration(ASTNode *node)
{
    if (isDeclared(node->data.function.name))
    {
        printf("Semantic Error: Function '%s' already declared.\n", node->data.function.name);
        return;
    }

    int paramCount = node->data.function.paramCount;

    // Allocate array for parameter types
    Type **paramTypes = malloc(sizeof(Type *) * paramCount);
    if (!paramTypes)
    {
        fprintf(stderr, "Memory allocation failed in checkFunctionDeclaration\n");
        exit(1);
    }

    // For each parameter node, get its type
    for (int i = 0; i < paramCount; i++)
    {
        // Assuming each param is an AST node representing a variable declaration or type
        ASTNode *paramNode = node->data.function.params[i];

        // Extract the type from paramNode.
        // This depends on how paramNode stores type info; assuming paramNode->data.varDecl.varType
        paramTypes[i] = getType(paramNode->data.varDecl.varType);
    }

    // Get the return type from the function node's returnType AST node
    Type *returnType = getType(node->data.function.returnType);

    Type *funcType = createFunctionType(paramTypes, paramCount, returnType);
    addSymbol(node->data.function.name, funcType);
}

/**
 * @brief Validates that a function call references a declared function,
 *        checks argument count and argument types.
 *
 * @param node The AST node representing a function call.
 *             Assumes the node is non-null and of type AST_FUNCTION_CALL,
 *             with arguments stored as a program node.
 */
void checkFunctionCall(ASTNode *node)
{
    if (!node || node->type != AST_FUNCTION_CALL) return;

    ASTNode *callee = node->data.call.callee;

    // Ensure callee is an identifier
    if (!callee || callee->type != AST_IDENTIFIER) {
        printf("Semantic Error: Function call target must be an identifier.\n");
        return;
    }

    const char *funcName = callee->data.identifier;
    SymbolTableEntry *entry = lookupSymbol(funcName);

    if (!entry) {
        printf("Semantic Error: Function '%s' called but not declared.\n", funcName);
        return;
    }

    if (!entry->type || entry->type->kind != TYPE_FUNCTION) {
        printf("Semantic Error: Symbol '%s' is not a function.\n", funcName);
        return;
    }

    int expected = entry->type->function.paramCount;
    int actual = node->data.call.argCount;

    if (expected != actual) {
        printf("Semantic Error: Function '%s' expects %d arguments, but got %d.\n",
               funcName, expected, actual);
        return;
    }

    // Check argument types
    for (int i = 0; i < expected; i++) {
        ASTNode *argNode = node->data.call.arguments[i];
        Type *argType = getType(argNode);
        Type *paramType = entry->type->function.paramTypes[i];

        if (!typeEquals(argType, paramType)) {
            printf("Semantic Error: Argument %d type mismatch in call to '%s'.\n", i + 1, funcName);
        }

        // Traverse the argument subtree
        traverse(argNode);
    }
}

/**
 * @brief Checks a while loop's condition type and traverses its body.
 *
 * @param node AST node of type AST_WHILE. Skipped if NULL or wrong type.
 *
 * @details 
 * - Ensures the condition is of type bool or int.
 * - Enters a new scope for the loop body and traverses it.
 */
void checkWhileLoop(ASTNode *node) {
    if (!node || node->type != AST_WHILE) return;

    // Check the condition
    Type *condType = getType(node->data.whileStmt.condition);
    if (condType->kind != TYPE_BOOL && condType->kind != TYPE_INT) {
        printf("Condition in while loop must be of type bool or int");
    }

    // Enter new scope for loop body
    enterScope();
    traverse(node->data.whileStmt.body);
    exitScope();
}

/**
 * @brief Validates a for loop's components and traverses its body.
 *
 * @param node AST node of type AST_FOR. Skipped if NULL or wrong type.
 *
 * @details 
 * - Enters a new scope for the loop.
 * - Checks the initializer, condition, increment, and body.
 * - Ensures the condition is of type bool or int.
 */
void checkForLoop(ASTNode *node) {
    if (!node || node->type != AST_FOR) return;

    enterScope();

    // Check initializer
    if (node->data.forStmt.init) {
        traverse(node->data.forStmt.init);
    }

    // Check condition
    if (node->data.forStmt.condition) {
        Type *condType = getType(node->data.forStmt.condition);
        if (condType->kind != TYPE_BOOL && condType->kind != TYPE_INT) {
            printf("Condition in for loop must be of type bool or int");
        }
    }

    // Check increment
    if (node->data.forStmt.increment) {
        traverse(node->data.forStmt.increment);
    }

    // Check body
    traverse(node->data.forStmt.body);

    exitScope();
}

/**
 * @brief Validates an array initializer's element types and checks against an expected type.
 *
 * @param node AST node of type AST_ARRAY_LITERAL. Skipped if NULL or wrong type.
 * @param expectedType Optional expected array type for compatibility check.
 *
 * @details 
 * - Ensures the array literal is non-empty.
 * - Verifies all elements are of the same type.
 * - If expectedType is provided, checks if element types match the declared type.
 */
void checkArrayInitializer(ASTNode *node, Type *expectedType) {
    if (!node || node->type != AST_ARRAY_LITERAL) return;

    if (node->data.arrayLiteral.elementCount == 0) {
        printf("Array literal cannot be empty");
        return;
    }

    Type *firstElemType = getType(node->data.arrayLiteral.elements[0]);

    for (int i = 1; i < node->data.arrayLiteral.elementCount; i++) {
        Type *elemType = getType(node->data.arrayLiteral.elements[i]);
        if (!typeEquals(firstElemType, elemType)) {
            printf("All elements in array literal must be of the same type");
            return;
        }
    }

    // If expectedType is provided, check compatibility
    if (expectedType && expectedType->kind == TYPE_ARRAY) {
        if (!typeEquals(expectedType->array.elementType, firstElemType)) {
            printf("Array initializer does not match declared array type");
        }
    }
}


/**
 * @brief Recursively traverses the AST and performs semantic analysis checks on each node.
 *
 * @param node The current AST node being visited.
 *             Assumes the node and its children are properly initialized.
 */
void traverse(ASTNode *node)
{
    if (!node) return;

    switch (node->type)
    {
    case AST_PROGRAM:
        for (int i = 0; i < node->data.program.count; i++)
            traverse(node->data.program.statements[i]);
        break;

    case AST_VAR_DECL:
        if (node->data.varDecl.varType)
            traverse(node->data.varDecl.varType);
        if (node->data.varDecl.initializer)
            traverse(node->data.varDecl.initializer);
        break;

    case AST_TYPE:
        // No traversal needed for type leaf
        break;

    case AST_NUMBER:
    case AST_STRING:
    case AST_IDENTIFIER:
        // Leaf nodes — no traversal needed
        break;

    case AST_BINARY_EXPR:
        traverse(node->data.binary.left);
        traverse(node->data.binary.right);
        break;

    case AST_UNARY_EXPR:
        traverse(node->data.unary.operand);
        break;

    case AST_FUNCTION:
        if (node->data.function.returnType)
            traverse(node->data.function.returnType);
        traverse(node->data.function.body);
        break;

    case AST_FUNCTION_CALL:
        traverse(node->data.call.callee);
        for (int i = 0; i < node->data.call.argCount; i++)
            traverse(node->data.call.arguments[i]);
        break;

    case AST_RETURN:
        if (node->data.returnStmt.expr)
            traverse(node->data.returnStmt.expr);
        break;

    case AST_IF:
        traverse(node->data.ifStmt.condition);
        traverse(node->data.ifStmt.thenBranch);
        if (node->data.ifStmt.elseBranch)
            traverse(node->data.ifStmt.elseBranch);
        break;

    case AST_PRINT_STATEMENT:
        traverse(node->data.printStmt.expr);
        break;

    case AST_WHILE:
        traverse(node->data.whileStmt.condition);
        traverse(node->data.whileStmt.body);
        break;

    case AST_FOR:
        if (node->data.forStmt.init)
            traverse(node->data.forStmt.init);
        if (node->data.forStmt.condition)
            traverse(node->data.forStmt.condition);
        if (node->data.forStmt.increment)
            traverse(node->data.forStmt.increment);
        traverse(node->data.forStmt.body);
        break;

    case AST_EXPR_STMT:
        if (node->data.ExprStmt.expr)
            traverse(node->data.ExprStmt.expr);
        break;

    case AST_ARRAY_LITERAL:
        for (int i = 0; i < node->data.arrayLiteral.elementCount; i++)
            traverse(node->data.arrayLiteral.elements[i]);
        break;

    default:
        // Unknown or unsupported node — no action
        break;
    }
}

/**
 * @brief Recursively traverses the AST and prints a debug-friendly
 *        representation of each node and its structure.
 *
 * This function is intended for debugging and visualization purposes.
 * It visits each node in the AST, printing its type and relevant data,
 * while maintaining indentation to reflect the tree structure.
 *
 * @param node  The current AST node being visited.
 *              Assumes the node and its children are properly initialized.
 * @param depth The current indentation depth for structured printing.
 *              Should start at 0 when first called.
 */
// Map ASTNodeType enums to strings
const char *ASTNodeTypeNames[] = {
    "AST_NUMBER",
    "AST_STRING",
    "AST_IDENTIFIER",
    "AST_BINARY_EXPR",
    "AST_UNARY_EXPR",
    "AST_VAR_DECL",
    "AST_RETURN",
    "AST_FUNCTION",
    "AST_IF",
    "AST_PROGRAM",
    "AST_TYPE",
    "AST_FUNCTION_CALL",
    "AST_PRINT_STATEMENT",
    "AST_ARRAY_LITERAL",
    "AST_WHILE",
    "AST_FOR",
    "AST_EXPR_STMT",
    "AST_TYPE_INT",
    "AST_TYPE_FLOAT",
    "AST_TYPE_BOOL",
    "AST_TYPE_STRING",
    "AST_TYPE_VOID",
    "AST_TYPE_ARRAY",
    "AST_TYPE_TUPLE",
    "AST_TYPE_STRUCT"
};

// Map TypeKind enums to strings
const char *TypeKindNames[] = {
    "TYPE_INT",
    "TYPE_FLOAT",
    "TYPE_BOOL",
    "TYPE_STRING",
    "TYPE_VOID",
    "TYPE_ARRAY",
    "TYPE_TUPLE",
    "TYPE_STRUCT",
    "TYPE_FUNCTION",
    "TYPE_UNKNOWN"
};
void debugTraverse(ASTNode *node)
{
    if (!node) return;

    switch (node->type)
    {
    case AST_PROGRAM:
        printf("Node: PROGRAM with %d statements\n", node->data.program.count);
        for (int i = 0; i < node->data.program.count; i++)
            debugTraverse(node->data.program.statements[i]);
        break;

    case AST_VAR_DECL:
        printf("Node: VAR_DECL - Name: %s\n", node->data.varDecl.varName);
        if (node->data.varDecl.varType)
        {
            printf("Var Type:\n");
            debugTraverse(node->data.varDecl.varType);
        }
        if (node->data.varDecl.initializer)
        {
            printf("Initializer:\n");
            debugTraverse(node->data.varDecl.initializer);
        }
        break;

    case AST_TYPE:
        {
            int kindIndex = node->data.type.typeKind - AST_TYPE_INT;
            // Defensive: check bounds
            if (kindIndex >= 0 && kindIndex < (sizeof(TypeKindNames)/sizeof(TypeKindNames[0])))
                printf("Node: TYPE - Kind: %s\n", TypeKindNames[kindIndex]);
            else
                printf("Node: TYPE - Kind: Unknown (%d)\n", node->data.type.typeKind);
        }
        break;

    case AST_NUMBER:
        printf("Node: NUMBER - Value: %d\n", node->data.number);
        break;

    case AST_STRING:
        printf("Node: STRING - Value: %s\n", node->data.string);
        break;

    case AST_IDENTIFIER:
        printf("Node: IDENTIFIER - Name: %s\n", node->data.identifier);
        break;

    case AST_BINARY_EXPR:
        printf("Node: BINARY_EXPR - Operator: %s\n", node->data.binary.op->lexeme);
        debugTraverse(node->data.binary.left);
        debugTraverse(node->data.binary.right);
        break;

    case AST_UNARY_EXPR:
        printf("Node: UNARY_EXPR - Operator: %s\n", node->data.unary.op->lexeme);
        debugTraverse(node->data.unary.operand);
        break;

    case AST_FUNCTION:
        printf("Node: FUNCTION - Name: %s, Params: %d\n", node->data.function.name, node->data.function.paramCount);
        printf("Return Type:\n");
        if (node->data.function.returnType)
            debugTraverse(node->data.function.returnType);
        printf("Body:\n");
        debugTraverse(node->data.function.body);
        break;

    case AST_FUNCTION_CALL:
        printf("Node: FUNCTION_CALL - Arguments: %d\n", node->data.call.argCount);
        printf("Callee:\n");
        debugTraverse(node->data.call.callee);
        for (int i = 0; i < node->data.call.argCount; i++)
            debugTraverse(node->data.call.arguments[i]);
        break;

    case AST_RETURN:
        printf("Node: RETURN\n");
        if (node->data.returnStmt.expr)
            debugTraverse(node->data.returnStmt.expr);
        break;

    case AST_IF:
        printf("Node: IF\n");
        printf("Condition:\n");
        debugTraverse(node->data.ifStmt.condition);
        printf("Then Branch:\n");
        debugTraverse(node->data.ifStmt.thenBranch);
        if (node->data.ifStmt.elseBranch)
        {
            printf("Else Branch:\n");
            debugTraverse(node->data.ifStmt.elseBranch);
        }
        break;

    case AST_PRINT_STATEMENT:
        printf("Node: PRINT_STATEMENT\n");
        debugTraverse(node->data.printStmt.expr);
        break;

    case AST_WHILE:
    printf("Node: WHILE\n");
    printf("Condition:\n");
    debugTraverse(node->data.whileStmt.condition);
    printf("Body:\n");
    debugTraverse(node->data.whileStmt.body);
    break;

    case AST_FOR:
        printf("Node: FOR\n");
        if (node->data.forStmt.init) {
            printf("Init:\n");
            debugTraverse(node->data.forStmt.init);
        }
        if (node->data.forStmt.condition) {
            printf("Condition:\n");
            debugTraverse(node->data.forStmt.condition);
        }
        if (node->data.forStmt.increment) {
            printf("Increment:\n");
            debugTraverse(node->data.forStmt.increment);
        }
        printf("Body:\n");
        debugTraverse(node->data.forStmt.body);
        break;

    case AST_EXPR_STMT:
    printf("Node: EXPR_STMT\n");
    if (node->data.ExprStmt.expr) {
        debugTraverse(node->data.ExprStmt.expr);
    }
    break;

    case AST_ARRAY_LITERAL:
        printf("Node: ARRAY_LITERAL with %d elements\n", node->data.arrayLiteral.elementCount);
        for (int i = 0; i < node->data.arrayLiteral.elementCount; i++) {
            debugTraverse(node->data.arrayLiteral.elements[i]);
        }
        break;

    default:
        // Unknown or unsupported node
        if (node->type >= AST_TYPE_INT && node->type <= AST_TYPE_STRUCT) {
            int kindIndex = node->type - AST_TYPE_INT;
            if (kindIndex >= 0 && kindIndex < (sizeof(TypeKindNames)/sizeof(TypeKindNames[0])))
                printf("Node: TYPE (Leaf) - Kind: %s\n", TypeKindNames[kindIndex]);
            else
                printf("Node: Unknown AST_TYPE Leaf (%d)\n", node->type);
        } else {
            printf("Node: Unknown type (%d)\n", node->type);
        }
        break;
    }
}
