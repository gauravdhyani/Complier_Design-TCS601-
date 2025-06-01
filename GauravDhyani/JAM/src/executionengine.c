#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "semanticanalyser.h"
#include "executionengine.h"


// -------------------------
// Helper Fucntions
// -------------------------

void freeValue(Value *v) {
    if (!v) return;
    if (v->type == VALUE_STRING && v->stringValue) {
        free(v->stringValue);
    } else if (v->type == VALUE_ARRAY && v->arrayValue.elements) {
        for (int i = 0; i < v->arrayValue.count; i++) {
            freeValue(v->arrayValue.elements[i]);
        }
        free(v->arrayValue.elements);
    }
    free(v);
}

typedef struct FunctionEntry {
    const char *name;
    ASTNode *functionNode;
    struct FunctionEntry *next;
} FunctionEntry;

FunctionEntry *functionRegistry = NULL;

void setFunctionEntry(const char *name, ASTNode *functionNode) {
    FunctionEntry *entry = malloc(sizeof(FunctionEntry));
    entry->name = strdup(name);
    entry->functionNode = functionNode;
    entry->next = functionRegistry;
    functionRegistry = entry;
}

ASTNode* getFunctionEntry(const char *name) {
    FunctionEntry *current = functionRegistry;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current->functionNode;
        }
        current = current->next;
    }
    return NULL;
}

Type* makeFunctionType(ASTNode* functionNode) {
    if (!functionNode || functionNode->type != AST_FUNCTION) {
        printf("Error: Invalid function node for type construction.\n");
        exit(1);
    }

    Type *t = malloc(sizeof(Type));
    if (!t) {
        perror("malloc failed");
        exit(1);
    }

    t->kind = TYPE_FUNCTION;
    t->function.paramCount = functionNode->data.function.paramCount;
    t->function.paramTypes = malloc(sizeof(Type*) * t->function.paramCount);
    if (!t->function.paramTypes) {
        perror("malloc failed");
        exit(1);
    }

    for (int i = 0; i < t->function.paramCount; i++) {
        ASTNode *param = functionNode->data.function.params[i];
        if (!param || param->type != AST_VAR_DECL || !param->data.varDecl.varType || param->data.varDecl.varType->type != AST_TYPE) {
            printf("Error: Invalid or missing type annotation for parameter in function '%s'.\n", functionNode->data.function.name);
            exit(1);
        }

        t->function.paramTypes[i] = getType(param->data.varDecl.varType);
        if (!t->function.paramTypes[i]) {
            printf("Error: Failed to resolve type for parameter in function '%s'.\n", functionNode->data.function.name);
            exit(1);
        }
    }

    if (!functionNode->data.function.returnType || functionNode->data.function.returnType->type != AST_TYPE) {
        printf("Error: Invalid or missing return type annotation for function '%s'.\n", functionNode->data.function.name);
        exit(1);
    }

    t->function.returnType = getType(functionNode->data.function.returnType);
    if (!t->function.returnType) {
        printf("Error: Failed to resolve return type for function '%s'.\n", functionNode->data.function.name);
        exit(1);
    }

    return t;
}

// -------------------------
// Environment Management
// -------------------------

EnvEntry *getEnvEntry(EnvEntry *env, char *name)
{
    //dumpEnvEntries(env);  // pass pointer, not dereferenced struct
    while (env)
    {
        if (strcmp(env->name, name) == 0)
            return env;
        env = env->next;
    }
    return NULL;
}


void dumpEnvEntries(EnvEntry *env) {
    printf("Environment contents:\n");
    EnvEntry *cur = env;
    while (cur) {
        printf(" - %s\n", cur->name);
        cur = cur->next;
    }
}

void addEnvEntry(EnvEntry **env, char *name, ASTNode *typeAnnotation) {
    EnvEntry *newEntry = malloc(sizeof(EnvEntry));
    newEntry->name = strdup(name);
    newEntry->typeAnnotation = typeAnnotation;
    newEntry->storedValue = NULL;   
    newEntry->next = *env;
    *env = newEntry;
}

void freeEnv(EnvEntry *env)
{
    while (env)
    {
        EnvEntry *next = env->next;
        free(env->name);
        if (env->typeAnnotation &&
            env->typeAnnotation->type == AST_TYPE &&
            env->typeAnnotation->data.type.typeKind == AST_TYPE_STRING &&
            env->value.stringValue) {
            free(env->value.stringValue);
        }
        if (env->storedValue) {
            // If it's a string or array, free its contents
            if (env->storedValue->type == VALUE_STRING && env->storedValue->stringValue) {
                free(env->storedValue->stringValue);
            } else if (env->storedValue->type == VALUE_ARRAY) {
                for (int i = 0; i < env->storedValue->arrayValue.count; i++) {
                    if (env->storedValue->arrayValue.elements[i]) {
                        free(env->storedValue->arrayValue.elements[i]);
                    }
                }
                free(env->storedValue->arrayValue.elements);
            }
            free(env->storedValue);
        }
        free(env);
        env = next;
    }
}


// -------------------------
// Call Stack Management
// -------------------------

CallStackEntry *callStack = NULL;
void pushCallStack(char *funcName, EnvEntry *env)
{
    CallStackEntry *entry = malloc(sizeof(CallStackEntry));
    if (!entry) {
        perror("malloc failed");
        exit(1);
    }

    entry->funcName = strdup(funcName);
    if (!entry->funcName) {
        perror("strdup failed");
        free(entry);
        exit(1);
    }

    entry->env = env;
    entry->next = callStack;
    callStack = entry;
}

void popCallStack()
{
    if (callStack)
    {
        CallStackEntry *entry = callStack;
        callStack = callStack->next;

        free(entry->funcName);
        // freeEnv(entry->env); // Uncomment if safe
        free(entry);
    }
}


// -------------------------
// Expression Evaluation
// -------------------------
Value* evaluateExpression(ASTNode *node, EnvEntry *env) {
    if (!node) {
        printf("Runtime Error: Null expression node.\n");
        exit(EXIT_FAILURE);
    }

    switch (node->type) {
        case AST_NUMBER: {
            Value *v = malloc(sizeof(Value));
            if (!v) { perror("malloc failed"); exit(EXIT_FAILURE); }
            v->type = VALUE_FLOAT;
            v->floatValue = (float)node->data.number;
            return v;
        }

        case AST_STRING: {
            Value *v = malloc(sizeof(Value));
            if (!v) { perror("malloc failed"); exit(EXIT_FAILURE); }
            v->type = VALUE_STRING;
            v->stringValue = strdup(node->data.string);
            if (!v->stringValue) { perror("strdup failed"); exit(EXIT_FAILURE); }
            return v;
        }

        case AST_IDENTIFIER: {
            EnvEntry *entry = getEnvEntry(env, node->data.identifier);
            if (!entry) {
                printf("Runtime Error: Undefined variable '%s'\n", node->data.identifier);
                exit(EXIT_FAILURE);
            }
            if (!entry->storedValue) {
                printf("Runtime Error: Variable '%s' used before being initialized.\n", node->data.identifier);
                exit(EXIT_FAILURE);
            }
            Value *copy = malloc(sizeof(Value));
            if (!copy) {
                perror("malloc failed");
                exit(EXIT_FAILURE);
            }
            *copy = *entry->storedValue;
            return copy;
        }

        case AST_BINARY_EXPR: {
            const char *op = node->data.binary.op->lexeme;

            if (strcmp(op, "=") == 0) {
                // Assignment operator

                ASTNode *leftNode = node->data.binary.left;
                ASTNode *rightNode = node->data.binary.right;

                if (leftNode->type != AST_IDENTIFIER) {
                    printf("Runtime Error: Left side of assignment must be a variable.\n");
                    exit(EXIT_FAILURE);
                }

                // Evaluate right side expression first
                Value *rightVal = evaluateExpression(rightNode, env);
                if (!rightVal) {
                    printf("Runtime Error: Failed to evaluate right side of assignment.\n");
                    exit(EXIT_FAILURE);
                }

                // Lookup the variable entry in environment
                EnvEntry *entry = getEnvEntry(env, leftNode->data.identifier);
                if (!entry) {
                    printf("Runtime Error: Variable '%s' not declared.\n", leftNode->data.identifier);
                    exit(EXIT_FAILURE);
                }

                // Free old stored value if any
                if (entry->storedValue) {
                    freeValue(entry->storedValue);
                }

                // Assign new value
                entry->storedValue = rightVal;

                // Return the assigned value
                return rightVal;
            }

            // For other binary operators, evaluate left and right as usual
            Value *left = evaluateExpression(node->data.binary.left, env);
            Value *right = evaluateExpression(node->data.binary.right, env);

            if ((left->type != VALUE_INT && left->type != VALUE_FLOAT) ||
                (right->type != VALUE_INT && right->type != VALUE_FLOAT)) {
                printf("Runtime Error: Binary operations require numeric operands.\n");
                exit(EXIT_FAILURE);
            }

            float l = (left->type == VALUE_FLOAT) ? left->floatValue : (float)left->intValue;
            float r = (right->type == VALUE_FLOAT) ? right->floatValue : (float)right->intValue;

            Value *v = malloc(sizeof(Value));
            if (!v) { perror("malloc failed"); exit(EXIT_FAILURE); }
            v->type = VALUE_FLOAT;

            if (strcmp(op, "<=") == 0) v->floatValue = l <= r ? 1.0f : 0.0f;
            else if (strcmp(op, ">=") == 0) v->floatValue = l >= r ? 1.0f : 0.0f;
            else if (strcmp(op, "<")  == 0) v->floatValue = l < r  ? 1.0f : 0.0f;
            else if (strcmp(op, ">")  == 0) v->floatValue = l > r  ? 1.0f : 0.0f;
            else if (strcmp(op, "==") == 0) v->floatValue = l == r ? 1.0f : 0.0f;
            else if (strcmp(op, "!=") == 0) v->floatValue = l != r ? 1.0f : 0.0f;
            else if (strcmp(op, "+")  == 0) v->floatValue = l + r;
            else if (strcmp(op, "-")  == 0) v->floatValue = l - r;
            else if (strcmp(op, "*")  == 0) v->floatValue = l * r;
            else if (strcmp(op, "/")  == 0) {
                if (r == 0.0f) {
                    printf("Runtime Error: Division by zero.\n");
                    exit(EXIT_FAILURE);
                }
                v->floatValue = l / r;
            } else {
                printf("Runtime Error: Unknown binary operator '%s'\n", op);
                exit(EXIT_FAILURE);
            }

            free(left);
            free(right);
            return v;
        }


        case AST_UNARY_EXPR: {
            Value *operand = evaluateExpression(node->data.unary.operand, env);
            const char *op = node->data.unary.op->lexeme;

            if (operand->type != VALUE_FLOAT) {
                printf("Runtime Error: Unary operations require float operand.\n");
                exit(EXIT_FAILURE);
            }

            Value *v = malloc(sizeof(Value));
            if (!v) { perror("malloc failed"); exit(EXIT_FAILURE); }
            v->type = VALUE_FLOAT;

            if (strcmp(op, "-") == 0) v->floatValue = -operand->floatValue;
            else if (strcmp(op, "!") == 0) v->floatValue = (!operand->floatValue) ? 1.0f : 0.0f;
            else {
                printf("Runtime Error: Unknown unary operator '%s'\n", op);
                exit(EXIT_FAILURE);
            }

            free(operand);
            return v;
        }

        case AST_FUNCTION_CALL: {
            if (!node->data.call.callee || node->data.call.callee->type != AST_IDENTIFIER) {
                printf("Runtime Error: Invalid function call callee.\n");
                exit(EXIT_FAILURE);
            }

            ASTNode *funcNode = getFunctionEntry(node->data.call.callee->data.identifier);
            if (!funcNode) {
                printf("Runtime Error: Undefined function '%s'\n", node->data.call.callee->data.identifier);
                exit(EXIT_FAILURE);
            }

            int argCount = node->data.call.argCount;
            ASTNode **argNodes = node->data.call.arguments;

            float result = executeFunction(funcNode, argNodes, argCount);
            Value *v = malloc(sizeof(Value));
            if (!v) { perror("malloc failed"); exit(EXIT_FAILURE); }
            v->type = VALUE_FLOAT;
            v->floatValue = result;
            return v;
        }

        case AST_ARRAY_LITERAL: {
            Value *v = malloc(sizeof(Value));
            if (!v) { perror("malloc failed"); exit(EXIT_FAILURE); }
            v->type = VALUE_ARRAY;
            v->arrayValue.count = node->data.arrayLiteral.elementCount;
            v->arrayValue.elements = malloc(sizeof(Value*) * v->arrayValue.count);
            if (!v->arrayValue.elements) { perror("malloc failed"); exit(EXIT_FAILURE); }

            for (int i = 0; i < v->arrayValue.count; i++) {
                v->arrayValue.elements[i] = evaluateExpression(node->data.arrayLiteral.elements[i], env);
            }

            return v;
        }

        default:
            printf("Runtime Error: Unsupported expression type %d\n", node->type);
            exit(EXIT_FAILURE);
    }
}

// -------------------------
// Statement Execution
// -------------------------
void executeStatement(ASTNode *node, EnvEntry **env, float *outReturnValue, bool *outHasReturned) {
    if (!node || (outHasReturned && *outHasReturned)) return;

    // Skip non-statement nodes
    if (node->type == AST_IDENTIFIER ||
        node->type == AST_NUMBER ||
        node->type == AST_STRING ||
        node->type == AST_BINARY_EXPR ||
        node->type == AST_UNARY_EXPR ||
        node->type == AST_ARRAY_LITERAL) {
        printf("Runtime Warning: Ignoring non-statement node of type %d\n", node->type);
        return;
    }

    switch (node->type) {
        case AST_VAR_DECL: {
                ASTNode *varTypeNode = node->data.varDecl.varType;
                if (!varTypeNode || varTypeNode->type != AST_TYPE) {
                    printf("Runtime Error: Invalid or missing type annotation for variable '%s'\n", node->data.varDecl.varName);
                    exit(EXIT_FAILURE);
                }

                addEnvEntry(env, node->data.varDecl.varName, varTypeNode);

                EnvEntry *entry = getEnvEntry(*env, node->data.varDecl.varName);
                if (!entry) {
                    printf("Runtime Error: Failed to find variable '%s' just after adding it!\n", node->data.varDecl.varName);
                    exit(EXIT_FAILURE);
                } 

                if (node->data.varDecl.initializer) {
                    Value *value = evaluateExpression(node->data.varDecl.initializer, *env);
                    if (!value) {
                        printf("Runtime Error: Failed to evaluate initializer for '%s'\n", node->data.varDecl.varName);
                        exit(EXIT_FAILURE);
                    }

                    // Allocate a new Value for storage
                    entry->storedValue = malloc(sizeof(Value));
                    if (!entry->storedValue) {
                        perror("malloc failed");
                        exit(EXIT_FAILURE);
                    }

                    switch (entry->typeAnnotation->data.type.typeKind) {
                        case AST_TYPE_INT:
                            if (value->type == VALUE_FLOAT || value->type == VALUE_INT) {
                                entry->storedValue->type = VALUE_INT;
                                entry->storedValue->intValue = (int)(value->type == VALUE_FLOAT ? value->floatValue : value->intValue);
                            } else {
                                printf("Runtime Error: Type mismatch assigning to int variable '%s'\n", node->data.varDecl.varName);
                                free(entry->storedValue);
                                entry->storedValue = NULL;
                            }
                            break;

                        case AST_TYPE_FLOAT:
                            if (value->type == VALUE_FLOAT || value->type == VALUE_INT) {
                                entry->storedValue->type = VALUE_FLOAT;
                                entry->storedValue->floatValue = (value->type == VALUE_FLOAT ? value->floatValue : (float)value->intValue);
                            } else {
                                printf("Runtime Error: Type mismatch assigning to float variable '%s'\n", node->data.varDecl.varName);
                                free(entry->storedValue);
                                entry->storedValue = NULL;
                            }
                            break;

                        case AST_TYPE_ARRAY:
                            if (value->type == VALUE_ARRAY) {
                                entry->storedValue->type = VALUE_ARRAY;
                                entry->storedValue->arrayValue = value->arrayValue; // shallow copy of pointer, or you can deep copy if needed
                            } else {
                                printf("Runtime Warning: Type mismatch assigning to array variable '%s'\n", node->data.varDecl.varName);
                                free(entry->storedValue);
                                entry->storedValue = NULL;
                            }
                            break;

                        default:
                            printf("Runtime Warning: Unsupported type for variable '%s'\n", node->data.varDecl.varName);
                            free(entry->storedValue);
                            entry->storedValue = NULL;
                            break;
                    }

                    free(value);  // now it's safe — we already copied or stored what we needed
                }

                break;
         }


        case AST_RETURN: {
            if (!node->data.returnStmt.expr) {
                *outReturnValue = 0.0f;
            } else {
                Value *retVal = evaluateExpression(node->data.returnStmt.expr, *env);
                if (retVal->type == VALUE_FLOAT)
                    *outReturnValue = retVal->floatValue;
                else if (retVal->type == VALUE_INT)
                    *outReturnValue = (float)retVal->intValue;
                else {
                    printf("Runtime Error: Return value must be scalar.\n");
                    exit(EXIT_FAILURE);
                }
                free(retVal);
            }
            *outHasReturned = true;
            break;
        }

        case AST_IF: {
            Value *condVal = evaluateExpression(node->data.ifStmt.condition, *env);
            if (condVal->type != VALUE_FLOAT && condVal->type != VALUE_INT) {
                printf("Runtime Error: Condition must be scalar.\n");
                exit(EXIT_FAILURE);
            }
            bool cond = (condVal->type == VALUE_FLOAT ? condVal->floatValue : condVal->intValue);
            free(condVal);

            if (cond)
                executeStatement(node->data.ifStmt.thenBranch, env, outReturnValue, outHasReturned);
            else if (node->data.ifStmt.elseBranch)
                executeStatement(node->data.ifStmt.elseBranch, env, outReturnValue, outHasReturned);
            break;
        }

        case AST_WHILE: {
            while (!(*outHasReturned)) {
                Value *condVal = evaluateExpression(node->data.whileStmt.condition, *env);
                if (condVal->type != VALUE_FLOAT && condVal->type != VALUE_INT) {
                    printf("Runtime Error: Condition must be scalar.\n");
                    exit(EXIT_FAILURE);
                }
                bool cond = (condVal->type == VALUE_FLOAT ? condVal->floatValue : condVal->intValue);
                free(condVal);

                if (!cond) break;
                executeStatement(node->data.whileStmt.body, env, outReturnValue, outHasReturned);
            }
            break;
        }

        case AST_FOR: {
            executeStatement(node->data.forStmt.init, env, outReturnValue, outHasReturned);
            while (!(*outHasReturned)) {
                Value *condVal = evaluateExpression(node->data.forStmt.condition, *env);
                if (condVal->type != VALUE_FLOAT && condVal->type != VALUE_INT) {
                    printf("Runtime Error: Condition must be scalar.\n");
                    exit(EXIT_FAILURE);
                }
                bool cond = (condVal->type == VALUE_FLOAT ? condVal->floatValue : condVal->intValue);
                free(condVal);

                if (!cond) break;
                executeStatement(node->data.forStmt.body, env, outReturnValue, outHasReturned);
                executeStatement(node->data.forStmt.increment, env, outReturnValue, outHasReturned);
            }
            break;
        }

        case AST_EXPR_STMT: {
        if (node->data.ExprStmt.expr->type == AST_BINARY_EXPR) {
            ASTNode *expr = node->data.ExprStmt.expr;
            if (strcmp(expr->data.binary.op->lexeme, "=") == 0) {
                // Left side must be an identifier
                if (expr->data.binary.left->type != AST_IDENTIFIER) {
                    printf("Runtime Error: Left side of assignment must be variable.\n");
                    exit(EXIT_FAILURE);
                }
                char *varName = expr->data.binary.left->data.identifier;
                Value *val = evaluateExpression(expr->data.binary.right, *env);
                EnvEntry *entry = getEnvEntry(*env, varName);
                if (!entry) {
                    printf("Runtime Error: Undefined variable '%s' in assignment.\n", varName);
                    exit(EXIT_FAILURE);
                }
                freeValue(entry->storedValue);  // free old value
                entry->storedValue = val;       // assign new value
                break;
            }
        }
        // Otherwise, evaluate expression normally
        Value *val = evaluateExpression(node->data.ExprStmt.expr, *env);
        freeValue(val);
        break;
    }


        case AST_FUNCTION_CALL: {
            Value *result = evaluateExpression(node, *env);
            free(result);  // Discard result in statement context
            break;
        }

        case AST_PRINT_STATEMENT: {
            ASTNode *expr = node->data.printStmt.expr;
            if (expr->type == AST_STRING) {
                printf("%s\n", expr->data.string);
            } else {
                Value *val = evaluateExpression(expr, *env);
                switch (val->type) {
                    case VALUE_INT:
                        printf("%d\n", val->intValue);
                        break;
                    case VALUE_FLOAT:
                        printf("%.2f\n", val->floatValue);
                        break;
                    case VALUE_STRING:
                        printf("%s\n", val->stringValue);
                        break;
                    case VALUE_ARRAY:
                        printf("[");
                        for (int i = 0; i < val->arrayValue.count; i++) {
                            Value *elem = val->arrayValue.elements[i];
                            if (elem->type == VALUE_INT)
                                printf("%d", elem->intValue);
                            else if (elem->type == VALUE_FLOAT)
                                printf("%.2f", elem->floatValue);
                            else
                                printf("?");
                            if (i < val->arrayValue.count - 1) printf(", ");
                        }
                        printf("]\n");
                        break;
                    default:
                        printf("?\n");
                        break;
                }
                free(val);
            }
            break;
        }

        case AST_FUNCTION: {
            addEnvEntry(env, node->data.function.name, node);  // Store full function node
            EnvEntry *entry = getEnvEntry(*env, node->data.function.name);
            if (entry)
                entry->functionNode = node;  // Optional if already stored
            break;
        }

        case AST_PROGRAM: {
            for (int i = 0; i < node->data.program.count && !(*outHasReturned); i++) {
                executeStatement(node->data.program.statements[i], env, outReturnValue, outHasReturned);
            }
            break;
        }

        default:
            printf("Runtime Error: Unsupported statement type %d\n", node->type);
            exit(EXIT_FAILURE);
    }
}

// -------------------------
// Function Execution
// -------------------------
float executeFunction(ASTNode *funcNode, ASTNode **args, int argCount) {
    if (!funcNode || funcNode->type != AST_FUNCTION) {
        printf("Runtime Error: Invalid function node.\n");
        return 0.0f;
    }

    if (argCount != funcNode->data.function.paramCount) {
        printf("Runtime Error: Function '%s' expects %d arguments, but got %d.\n",
               funcNode->data.function.name, funcNode->data.function.paramCount, argCount);
        return 0.0f;
    }

    EnvEntry *localEnv = NULL;

    for (int i = 0; i < argCount; i++) {
        ASTNode *paramNode = funcNode->data.function.params[i];
        ASTNode *argNode = args[i];

        if (!paramNode || paramNode->type != AST_VAR_DECL) {
            printf("Runtime Error: Invalid parameter declaration in function '%s'.\n",
                   funcNode->data.function.name);
            continue;
        }

        if (!paramNode->data.varDecl.varType || paramNode->data.varDecl.varType->type != AST_TYPE) {
            printf("Runtime Error: Missing or invalid type annotation for parameter '%s' in function '%s'.\n",
                   paramNode->data.varDecl.varName, funcNode->data.function.name);
            continue;
        }

        addEnvEntry(&localEnv, paramNode->data.varDecl.varName, paramNode->data.varDecl.varType);

        Value *argValue = evaluateExpression(argNode, callStack ? callStack->env : NULL);
        EnvEntry *entry = getEnvEntry(localEnv, paramNode->data.varDecl.varName);

        if (!entry) {
            printf("Runtime Error: Failed to bind argument to parameter '%s'.\n",
                   paramNode->data.varDecl.varName);
            free(argValue);
            continue;
        }

        ASTNodeType expectedType = entry->typeAnnotation->data.type.typeKind;

        switch (expectedType) {
            case AST_TYPE_INT:
                if (argValue->type == VALUE_INT || argValue->type == VALUE_FLOAT) {
                    entry->storedValue = argValue;
                } else {
                    printf("Runtime Warning: Type mismatch for parameter '%s'. Expected int.\n", paramNode->data.varDecl.varName);
                    free(argValue);
                }
                break;

            case AST_TYPE_FLOAT:
                if (argValue->type == VALUE_FLOAT || argValue->type == VALUE_INT) {
                    entry->storedValue = argValue;
                } else {
                    printf("Runtime Warning: Type mismatch for parameter '%s'. Expected float.\n", paramNode->data.varDecl.varName);
                    free(argValue);
                }
                break;

            case AST_TYPE_ARRAY:
                if (argValue->type == VALUE_ARRAY) {
                    entry->storedValue = argValue;
                } else {
                    printf("Runtime Warning: Type mismatch for parameter '%s'. Expected array.\n", paramNode->data.varDecl.varName);
                    free(argValue);
                }
                break;

            default:
                printf("Runtime Warning: Unsupported parameter type (kind: %d) for '%s'.\n",
                       expectedType, paramNode->data.varDecl.varName);
                free(argValue);
                break;
        }
    }

    pushCallStack(funcNode->data.function.name, localEnv);

    float returnValue = 0.0f;
    bool hasReturned = false;

    executeStatement(funcNode->data.function.body, &localEnv, &returnValue, &hasReturned);

    popCallStack();
    freeEnv(localEnv);

    return returnValue;
}

// -------------------------
// Top-Level Execution
// -------------------------
void execute(ASTNode *node)
{
    if (!node)
        return;

    // Global environment for the entire program execution
    static EnvEntry *globalEnv = NULL;

    float returnValue = 0.0f;
    bool hasReturned = false;

    switch (node->type)
    {
        case AST_PROGRAM:
        {
            // First pass: register all functions in the global environment
            for (int i = 0; i < node->data.program.count; i++)
            {
                ASTNode *stmt = node->data.program.statements[i];
                if (!stmt)
                    continue;

                if (stmt->type == AST_FUNCTION)
                {
                    setFunctionEntry(stmt->data.function.name, stmt);
                }
            }

            // Second pass: execute non-function global statements
            for (int i = 0; i < node->data.program.count; i++)
            {
                ASTNode *stmt = node->data.program.statements[i];
                if (!stmt || stmt->type == AST_FUNCTION)
                    continue;

                executeStatement(stmt, &globalEnv, &returnValue, &hasReturned);

                if (hasReturned) {
                    printf("Warning: Return statement executed at global scope.\n");
                    hasReturned = false;
                }
            }
            break;
        }

        case AST_FUNCTION:
            printf("Warning: Function '%s' found outside program block.\n", node->data.function.name);
            break;

        case AST_VAR_DECL:
        case AST_IF:
        case AST_RETURN:
        case AST_PRINT_STATEMENT:
        case AST_UNARY_EXPR:
        case AST_BINARY_EXPR:
        case AST_IDENTIFIER:
        case AST_NUMBER:
        case AST_STRING:
        case AST_FOR:
        case AST_WHILE:
        case AST_FUNCTION_CALL:
        case AST_ARRAY_LITERAL:
            executeStatement(node, &globalEnv, &returnValue, &hasReturned);
            break;

        default:
            printf("Runtime Error: Unsupported top-level node type %d\n", node->type);
            break;
    }
}

 // -------------------------
 // Script Runner
 // -------------------------

 /**
  * Loads and runs a JAM script from file.
  *
  * @param filename Path to the script file.
  * @return 0 on success, 1 on failure.
  */
int run_jam_script(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Script open error");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *source_code = malloc(size + 1);
    fread(source_code, 1, size, file);
    source_code[size] = '\0';
    fclose(file);

    // Lexing
    initlexer(source_code);
    Token** tokens = NULL;
    int count = 0, cap = 0;
    while (1) {
        Token* t = get_next_token();
        if (count == cap) {
            cap = cap ? cap * 2 : 8;
            tokens = realloc(tokens, cap * sizeof(Token*));
        }
        tokens[count++] = t;
        if (t->type == TOKEN_EOF) break;
    }

    // Parse
    Parser parser;
    initParser(&parser, tokens, count);
    ASTNode *ast = parseProgram(&parser);
    if (!ast) {
        fprintf(stderr, "Parsing failed.\n");
        free(source_code);
        for (int i = 0; i < count; i++) {
            free(tokens[i]->lexeme);
            free(tokens[i]);
        }
        free(tokens);
        return 1;
    }

    // Semantic Analysis
    enterScope();
    traverse(ast);
    exitScope();

    // Execution
    printf("\n===== Execution =====\n");
    execute(ast);

    // Cleanup
    freeAST(ast);
    for (int i = 0; i < count; i++) {
        free(tokens[i]->lexeme);
        free(tokens[i]);
    }
    free(tokens);
    free(source_code);
    return 0;
}


  