#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// --- Forward declarations ---
ASTNode *parseProgram(Parser *p);
ASTNode *parsePrintStatement(Parser *p);
static ASTNode *parseFunction(Parser *p);
static ASTNode *parseStatement(Parser *p);
static ASTNode *parseVarDecl(Parser *p);
static ASTNode *parseExpression(Parser *p);
static ASTNode *parseUnary(Parser *p);
static ASTNode *parseBinaryExpr(Parser *p, int prec);
static ASTNode *parsePrimary(Parser *p);
static ASTNode *parseIfStatement(Parser *p);
static ASTNode *parseType(Parser *p);
static ASTNode *parseBlock(Parser *p);
static int getPrecedence(Token *op);
bool check(Parser *p, TokenType t);
static ASTNode* parseAssignment(Parser *p);
static ASTNode *parseWhileStatement(Parser *p);
static ASTNode *parseForStatement(Parser *p);
static bool isAtEnd(Parser *p);

static int getPrecedence(Token *op) {
    // Return an int indicating operator precedence, for example:
    switch (op->type) {
        case TOKEN_OPERATOR_OR: return 1;
        case TOKEN_OPERATOR_AND: return 2;
        case TOKEN_OPERATOR_EQ:
        case TOKEN_OPERATOR_NEQ: return 3;
        case TOKEN_OPERATOR_LT:
        case TOKEN_OPERATOR_LTE:
        case TOKEN_OPERATOR_GT:
        case TOKEN_OPERATOR_GTE: return 4;
        case TOKEN_OPERATOR_PLUS:
        case TOKEN_OPERATOR_MINUS: return 5;
        case TOKEN_OPERATOR_MUL:
        case TOKEN_OPERATOR_DIV:
        case TOKEN_OPERATOR_MOD: return 6;
        default: return 0; // no precedence
    }
}

// --- Lexer lookahead helpers ---
static Token *peek(Parser *p)
{
    return (p->current < p->tokenCount
                ? p->tokens[p->current]
                : NULL);
}
static Token *advance(Parser *p)
{
    return (p->current < p->tokenCount
                ? p->tokens[p->current++]
                : NULL);
}
static int match(Parser *p, TokenType t)
{
    Token *tok = peek(p);
    if (tok && tok->type == t)
    {
        advance(p);
        return 1;
    }
    return 0;
}
static void consume(Parser *p, TokenType t, const char *msg)
{
    if (!match(p, t))
    {
        Token *tok = peek(p);
        fprintf(stderr,
                "Parse error at line %d col %d: %s (got '%s')\n",
                tok ? tok->line : -1,
                tok ? tok->col : -1,
                msg,
                tok ? tok->lexeme : "EOF");
        exit(1);
    }
}

bool check(Parser *p, TokenType t) {
    Token *curr = peek(p);
    if (!curr) return false;
    return curr->type == t;
}

// --- AST node constructors ---
static ASTNode* makeNode(ASTNodeType type) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    node->type = type;
    memset(&node->data, 0, sizeof(node->data));  // zero init union
    return node;
}

static ASTNode *binaryNode(ASTNode *left, Token *op, ASTNode *right)
{
    ASTNode *n = makeNode(AST_BINARY_EXPR);
    n->data.binary.left = left;
    n->data.binary.op = op;
    n->data.binary.right = right;
    return n;
}

// --- Parsing functions ---

// primary ::= NUMBER | STRING | IDENTIFIER | '(' expression ')'
static ASTNode *parsePrimary(Parser *p)
{
    Token *t = peek(p);
    if (!t)
        return NULL;

    if (t->type == TOKEN_NUMBER)
    {
        advance(p);
        ASTNode *node = makeNode(AST_NUMBER);
        node->data.number = atoi(t->lexeme);
        return node;
    }

    if (t->type == TOKEN_STRING)
    {
        advance(p);
        ASTNode *node = makeNode(AST_STRING);
        node->data.string = strdup(t->lexeme);
        return node;
    }

    if (t->type == TOKEN_IDENTIFIER)
    {
        char *idName = strdup(t->lexeme);
        advance(p);

        if (match(p, TOKEN_DELIM_OPEN_PAREN))
        {
            ASTNode **args = NULL;
            int argCount = 0;

            if (!check(p, TOKEN_DELIM_CLOSE_PAREN))
            {
                do
                {
                    ASTNode *arg = parseExpression(p);
                    args = realloc(args, sizeof(ASTNode *) * (argCount + 1));
                    args[argCount++] = arg;
                } while (match(p, TOKEN_DELIM_COMMA));
            }

            consume(p, TOKEN_DELIM_CLOSE_PAREN, "Expected ')' after function call arguments");

            ASTNode *callNode = makeNode(AST_FUNCTION_CALL);
            callNode->data.call.callee = makeNode(AST_IDENTIFIER);
            callNode->data.call.callee->data.identifier = idName;
            callNode->data.call.arguments = args;
            callNode->data.call.argCount = argCount;
            return callNode;
        }

        ASTNode *idNode = makeNode(AST_IDENTIFIER);
        idNode->data.identifier = idName;
        return idNode;
    }

    if (match(p, TOKEN_DELIM_OPEN_PAREN))
    {
        ASTNode *expr = parseExpression(p);
        consume(p, TOKEN_DELIM_CLOSE_PAREN, "Expected ')'");
        return expr;
    }
    if (match(p, TOKEN_DELIM_OPEN_SQUARE)) {
        ASTNode **elements = NULL;
        int count = 0;

        if (!check(p, TOKEN_DELIM_CLOSE_SQUARE)) {
            do {
                ASTNode *elem = parseExpression(p);
                elements = realloc(elements, sizeof(ASTNode *) * (count + 1));
                elements[count++] = elem;
            } while (match(p, TOKEN_DELIM_COMMA));
        }

        consume(p, TOKEN_DELIM_CLOSE_SQUARE, "Expected ']' after array literal");

        ASTNode *arrayNode = makeNode(AST_ARRAY_LITERAL);
        arrayNode->data.arrayLiteral.elements = elements;
        arrayNode->data.arrayLiteral.elementCount = count;
        return arrayNode;
    }

    fprintf(stderr, "Unexpected token '%s' in primary expression\n", t->lexeme);
    exit(1);
}

// expression ::= term ( (+|-) term )*
static ASTNode* parseExpression(Parser *p) {
    return parseAssignment(p);
}

static Token *previous(Parser *p) {
    if (p->current > 0)
        return p->tokens[p->current - 1];
    return NULL;
}

static ASTNode* parseAssignment(Parser *p) {
    ASTNode *left = parseBinaryExpr(p, 0);

    if (match(p, TOKEN_OPERATOR_ASSIGN)) { // '='
        if (left->type != AST_IDENTIFIER) {
            fprintf(stderr, "Invalid assignment target\n");
            exit(1);
        }

        Token *assignOp = previous(p); // '=' token
        ASTNode *right = parseAssignment(p); // right-associative

        ASTNode *node = makeNode(AST_BINARY_EXPR);
        node->data.binary.op = assignOp;
        node->data.binary.left = left;
        node->data.binary.right = right;
        return node;
    }

    return left;
}

static ASTNode* parseUnary(Parser *p) {
    Token *t = peek(p);
    if (!t) return NULL;

    if (t->type == TOKEN_OPERATOR_NOT || t->type == TOKEN_OPERATOR_MINUS) {
        advance(p);
        ASTNode *operand = parseUnary(p);
        ASTNode *node = makeNode(AST_UNARY_EXPR);
        node->data.unary.op = t;
        node->data.unary.operand = operand;
        return node;
    }

    return parsePrimary(p);
}

static ASTNode* parseBinaryExpr(Parser *p, int prec) {
    ASTNode *left = parseUnary(p);

    while (1) {
        Token *op = peek(p);
        int opPrec = getPrecedence(op);

        if (opPrec == 0 || opPrec <= prec)
            break;

        advance(p);
        ASTNode *right = parseBinaryExpr(p, opPrec);

        left = binaryNode(left, op, right);
    }

    return left;
}

// varDecl ::= 'var' IDENT [ '=' expression ] ';'
static ASTNode *parseVarDecl(Parser *p)
{
    advance(p); // consume 'var'
    Token *id = peek(p);
    consume(p, TOKEN_IDENTIFIER, "Expected variable name");

    ASTNode *varDecl = makeNode(AST_VAR_DECL);
    varDecl->data.varDecl.varName = strdup(id->lexeme);

    // Parse optional ': Type'
    if (match(p, TOKEN_DELIM_COLON)) {
        varDecl->data.varDecl.varType = parseType(p);  // Add varType field to AST_VAR_DECL
    } else {
        varDecl->data.varDecl.varType = NULL;
    }

    if (match(p, TOKEN_OPERATOR_ASSIGN))
    {
        varDecl->data.varDecl.initializer = parseExpression(p);
    }

    consume(p, TOKEN_DELIM_SEMICOLON, "Expected ';' after var declaration");
    return varDecl;
}

// statement ::= 'return' expression ';'
//             | varDecl
//             | expression ';'
static ASTNode *parseStatement(Parser *p)
{
    Token *t = peek(p);
    if (!t)
        return NULL;

    if (t->type == TOKEN_KEYWORD_RETURN) {
        advance(p);
        ASTNode *n = makeNode(AST_RETURN);
        n->data.returnStmt.expr = parseExpression(p);
        consume(p, TOKEN_DELIM_SEMICOLON, "Expected ';' after return");
        return n;
    }

    if (t->type == TOKEN_KEYWORD_VAR) {
        return parseVarDecl(p);
    }

    if (t->type == TOKEN_KEYWORD_IF) {
        return parseIfStatement(p);
    }

    if (t->type == TOKEN_KEYWORD_LOOP) {
        return parseWhileStatement(p);
    }

    if (t->type == TOKEN_KEYWORD_FORLOOP) {
        return parseForStatement(p);
    }

    if (t->type == TOKEN_IDENTIFIER && strcmp(t->lexeme, "print") == 0) {
        return parsePrintStatement(p);
    }

    // If none of the above, it's an expression statement
    ASTNode *expr = parseExpression(p);
    consume(p, TOKEN_DELIM_SEMICOLON, "Expected ';' after expression");

    // Wrap the expression in an expression statement node
    ASTNode *stmt = makeNode(AST_EXPR_STMT);
    stmt->data.ExprStmt.expr = expr;
    return stmt;
}

// function ::= 'fn' IDENT '(' ( param (',' param)* )? ')' ( '->' Type )? Block
// param ::= IDENT ':' Type
static ASTNode *parseFunction(Parser *p)
{
    advance(p); // consume 'fn'
    Token *name = peek(p);
    consume(p, TOKEN_IDENTIFIER, "Expected function name");
    ASTNode *fn = makeNode(AST_FUNCTION);
    fn->data.function.name = strdup(name->lexeme);

    consume(p, TOKEN_DELIM_OPEN_PAREN, "Expected '(' after function name");

    // parse parameters
    fn->data.function.params = NULL; // new field: array of AST_VAR_DECL or similar
    fn->data.function.paramCount = 0;

    if (!match(p, TOKEN_DELIM_CLOSE_PAREN))
    {
        do {
            // parse param
            Token* paramName = peek(p);
            consume(p, TOKEN_IDENTIFIER, "Expected parameter name");

            consume(p, TOKEN_DELIM_COLON, "Expected ':' after parameter name");

            ASTNode* paramType = parseType(p);

            ASTNode* paramNode = makeNode(AST_VAR_DECL);
            paramNode->data.varDecl.varName = strdup(paramName->lexeme);
            paramNode->data.varDecl.varType = paramType;
            paramNode->data.varDecl.initializer = NULL;

            fn->data.function.paramCount++;
            fn->data.function.params = realloc(
                fn->data.function.params,
                sizeof(ASTNode*) * fn->data.function.paramCount);
            fn->data.function.params[fn->data.function.paramCount - 1] = paramNode;
        } while (match(p, TOKEN_DELIM_COMMA));

        consume(p, TOKEN_DELIM_CLOSE_PAREN, "Expected ')' after parameters");
    }

    // parse optional return type
    if (match(p, TOKEN_ARROW)) // '->'
    {
        fn->data.function.returnType = parseType(p);  // new field
    }
    else
    {
        fn->data.function.returnType = NULL;  // void or no explicit return type
    }

    // parse function body block
    consume(p, TOKEN_DELIM_OPEN_BRACE, "Expected '{' to begin function body");
    fn->data.function.body = makeNode(AST_PROGRAM);
    fn->data.function.body->data.program.statements = NULL;
    fn->data.function.body->data.program.count = 0;

    while (!match(p, TOKEN_DELIM_CLOSE_BRACE))
    {
        ASTNode *stmt = parseStatement(p);
        int c = fn->data.function.body->data.program.count++;
        fn->data.function.body->data.program.statements = realloc(
            fn->data.function.body->data.program.statements,
            fn->data.function.body->data.program.count * sizeof(ASTNode *));
        fn->data.function.body->data.program.statements[c] = stmt;
    }
    return fn;
}

static ASTNode* parseType(Parser* p) {
    Token* t = peek(p);
    if (!t) {
        fprintf(stderr, "Unexpected EOF while parsing type\n");
        exit(1);
    }

    // Base types (keywords)
    switch (t->type) {
        case TOKEN_KEYWORD_INT:
            advance(p);
            {
                ASTNode* node = makeNode(AST_TYPE);
                node->data.type.typeKind = AST_TYPE_INT;
                return node;
            }
        case TOKEN_KEYWORD_FLOAT:
            advance(p);
            {
                ASTNode* node = makeNode(AST_TYPE);
                node->data.type.typeKind = AST_TYPE_FLOAT;
                return node;
            }
        case TOKEN_KEYWORD_BOOL:
            advance(p);
            {
                ASTNode* node = makeNode(AST_TYPE);
                node->data.type.typeKind = AST_TYPE_BOOL;
                return node;
            }
        case TOKEN_KEYWORD_STRING:
            advance(p);
            {
                ASTNode* node = makeNode(AST_TYPE);
                node->data.type.typeKind = AST_TYPE_STRING;
                return node;
            }
        case TOKEN_KEYWORD_VOID:
            advance(p);
            {
                ASTNode* node = makeNode(AST_TYPE);
                node->data.type.typeKind = AST_TYPE_VOID;
                return node;
            }
        default:
            break;
    }

    // Array: [Type]
    if (match(p, TOKEN_DELIM_OPEN_SQUARE)) {
        ASTNode* elemType = parseType(p);
        consume(p, TOKEN_DELIM_CLOSE_SQUARE, "Expected ']' after array element type");

        ASTNode* arrayNode = makeNode(AST_TYPE);
        arrayNode->data.type.typeKind = AST_TYPE_ARRAY;
        arrayNode->data.type.elementType = elemType;
        return arrayNode;
    }

    // Tuple: (Type, Type, ...)
    if (match(p, TOKEN_DELIM_OPEN_PAREN)) {
        ASTNode** elems = NULL;
        int count = 0;

        do {
            ASTNode* elemType = parseType(p);
            elems = realloc(elems, sizeof(ASTNode*) * (count + 1));
            elems[count++] = elemType;
        } while (match(p, TOKEN_DELIM_COMMA));

        consume(p, TOKEN_DELIM_CLOSE_PAREN, "Expected ')' after tuple types");

        ASTNode* tupleNode = makeNode(AST_TYPE);
        tupleNode->data.type.typeKind = AST_TYPE_TUPLE;
        tupleNode->data.type.tuple.elementTypes = elems;
        tupleNode->data.type.tuple.elementCount = count;
        return tupleNode;
    }

    // Struct type usage: "struct IDENT"
    if (match(p, TOKEN_KEYWORD_STRUCT)) {
        Token* name = peek(p);
        consume(p, TOKEN_IDENTIFIER, "Expected struct name");

        ASTNode* structTypeNode = makeNode(AST_TYPE);
        structTypeNode->data.type.typeKind = AST_TYPE_STRUCT;
        structTypeNode->data.type.structType.name = strdup(name->lexeme);
        structTypeNode->data.type.structType.fields = NULL;
        structTypeNode->data.type.structType.fieldCount = 0;
        // Note: parsing of the struct body happens separately in parseStruct()
        return structTypeNode;
    }

    fprintf(stderr, "Unknown type: %s\n", t->lexeme);
    exit(1);
}


void initParser(Parser *p, Token **tokens, int tokenCount)
{
    p->tokens = tokens;
    p->tokenCount = tokenCount;
    p->current = 0;
}

ASTNode *parseProgram(Parser *p)
{
    ASTNode *prog = makeNode(AST_PROGRAM);
    prog->data.program.statements = NULL;
    prog->data.program.count = 0;

    while (peek(p) && peek(p)->type != TOKEN_EOF)
    {
        ASTNode *node;
        if (peek(p)->type == TOKEN_KEYWORD_FN)
        {
            node = parseFunction(p);
        }
        else
        {
            node = parseStatement(p);
        }
        int c = prog->data.program.count++;
        prog->data.program.statements = realloc(
            prog->data.program.statements,
            prog->data.program.count * sizeof(ASTNode *));
        prog->data.program.statements[c] = node;
    }
    return prog;
}

static ASTNode *parseIfStatement(Parser *p)
{
    advance(p); // consume 'if'
    consume(p, TOKEN_DELIM_OPEN_PAREN, "Expected '(' after 'if'");

    ASTNode *condition = parseExpression(p);

    consume(p, TOKEN_DELIM_CLOSE_PAREN, "Expected ')' after condition");
    consume(p, TOKEN_DELIM_OPEN_BRACE, "Expected '{' for 'if' body");

    ASTNode *thenBranch = makeNode(AST_PROGRAM);
    thenBranch->data.program.statements = NULL;
    thenBranch->data.program.count = 0;

    while (!match(p, TOKEN_DELIM_CLOSE_BRACE))
    {
        ASTNode *stmt = parseStatement(p);
        int c = thenBranch->data.program.count++;
        thenBranch->data.program.statements = realloc(
            thenBranch->data.program.statements,
            thenBranch->data.program.count * sizeof(ASTNode *));
        thenBranch->data.program.statements[c] = stmt;
    }

    ASTNode *elseBranch = NULL;
    if (match(p, TOKEN_KEYWORD_ELSE))
    {
        consume(p, TOKEN_DELIM_OPEN_BRACE, "Expected '{' for 'else' body");

        elseBranch = makeNode(AST_PROGRAM);
        elseBranch->data.program.statements = NULL;
        elseBranch->data.program.count = 0;

        while (!match(p, TOKEN_DELIM_CLOSE_BRACE))
        {
            ASTNode *stmt = parseStatement(p);
            int c = elseBranch->data.program.count++;
            elseBranch->data.program.statements = realloc(
                elseBranch->data.program.statements,
                elseBranch->data.program.count * sizeof(ASTNode *));
            elseBranch->data.program.statements[c] = stmt;
        }
    }

    ASTNode *node = makeNode(AST_IF);
    node->data.ifStmt.condition = condition;
    node->data.ifStmt.thenBranch = thenBranch;
    node->data.ifStmt.elseBranch = elseBranch;

    return node;
}


static bool isAtEnd(Parser *p) {
    return peek(p) == NULL;
}

static ASTNode *parseBlock(Parser *p) {
    consume(p, TOKEN_DELIM_OPEN_BRACE, "Expected '{' to start block");

    ASTNode **statements = NULL;
    int count = 0;

    while (!check(p, TOKEN_DELIM_CLOSE_BRACE) && !isAtEnd(p)) {
        ASTNode *stmt = parseStatement(p);
        statements = realloc(statements, sizeof(ASTNode *) * (count + 1));
        statements[count++] = stmt;
    }

    consume(p, TOKEN_DELIM_CLOSE_BRACE, "Expected '}' to end block");

    ASTNode *block = makeNode(AST_PROGRAM);
    block->data.program.statements = statements;
    block->data.program.count = count;
    return block;
}

static ASTNode *parseWhileStatement(Parser *p) {
    consume(p, TOKEN_KEYWORD_LOOP, "Expected 'while'");
    consume(p, TOKEN_DELIM_OPEN_PAREN, "Expected '(' after 'while'");
    ASTNode *condition = parseExpression(p);
    consume(p, TOKEN_DELIM_CLOSE_PAREN, "Expected ')' after condition");

    ASTNode *body = parseBlock(p); // or parseStatement(p) if you allow single-line bodies

    ASTNode *node = makeNode(AST_WHILE);
    node->data.whileStmt.condition = condition;
    node->data.whileStmt.body = body;
    return node;
}

static ASTNode *parseForStatement(Parser *p) {
    consume(p, TOKEN_KEYWORD_FORLOOP, "Expected 'for'");
    consume(p, TOKEN_DELIM_OPEN_PAREN, "Expected '(' after 'for'");

    ASTNode *init = parseStatement(p); 
    ASTNode *condition = parseExpression(p);
    consume(p, TOKEN_DELIM_SEMICOLON, "Expected ';' after loop condition");
    ASTNode *increment = parseExpression(p);
    consume(p, TOKEN_DELIM_CLOSE_PAREN, "Expected ')' after increment");

    ASTNode *body = parseBlock(p); 

    ASTNode *node = makeNode(AST_FOR);
    node->data.forStmt.init = init;
    node->data.forStmt.condition = condition;
    node->data.forStmt.increment = increment;
    node->data.forStmt.body = body;
    return node;
}

ASTNode *parsePrintStatement(Parser *p) {
    // Consume 'print' identifier
    Token *printToken = p->tokens[p->current];
    if (strcmp(printToken->lexeme, "print") != 0) {
        printf("Expected 'print' statement at line %d, col %d\n", printToken->line, printToken->col);
        exit(1);
    }
    p->current++;

    // Consume '('
    if (p->tokens[p->current]->type != TOKEN_DELIM_OPEN_PAREN) {
        printf("Expected '(' after 'print' at line %d, col %d\n", p->tokens[p->current]->line, p->tokens[p->current]->col);
        exit(1);
    }
    p->current++;

    // Parse expression to print
    ASTNode *expr = parseExpression(p);

    // Consume ')'
    if (p->tokens[p->current]->type != TOKEN_DELIM_CLOSE_PAREN) {
        printf("Expected ')' after expression in 'print' at line %d, col %d\n", p->tokens[p->current]->line, p->tokens[p->current]->col);
        exit(1);
    }
    p->current++;

    // Consume ';'
    if (p->tokens[p->current]->type != TOKEN_DELIM_SEMICOLON) {
        printf("Expected ';' after 'print' statement at line %d, col %d\n", p->tokens[p->current]->line, p->tokens[p->current]->col);
        exit(1);
    }
    p->current++;

    // Create AST node
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_PRINT_STATEMENT;
    node->data.printStmt.expr = expr;
    return node;
}

void freeAST(ASTNode *node)
{
    if (!node)
        return;

    switch (node->type)
    {
    case AST_PRINT_STATEMENT:
        freeAST(node->data.printStmt.expr);
        break;

    case AST_NUMBER:
        // Nothing to free
        break;

    case AST_STRING:
        free(node->data.string);
        break;

    case AST_IDENTIFIER:
        free(node->data.identifier);
        break;

    case AST_BINARY_EXPR:
        freeAST(node->data.binary.left);
        freeAST(node->data.binary.right);
        break;

    case AST_VAR_DECL:
        free(node->data.varDecl.varName);
        if (node->data.varDecl.varType)
            freeAST(node->data.varDecl.varType);
        if (node->data.varDecl.initializer)
            freeAST(node->data.varDecl.initializer);
        break;

    case AST_RETURN:
        freeAST(node->data.returnStmt.expr);
        break;

    case AST_FUNCTION:
        free(node->data.function.name);
        for (int i = 0; i < node->data.function.paramCount; i++)
            freeAST(node->data.function.params[i]);
        free(node->data.function.params);
        if (node->data.function.returnType)
            freeAST(node->data.function.returnType);
        freeAST(node->data.function.body);
        break;

    case AST_IF:
        freeAST(node->data.ifStmt.condition);
        freeAST(node->data.ifStmt.thenBranch);
        if (node->data.ifStmt.elseBranch)
            freeAST(node->data.ifStmt.elseBranch);
        break;

    case AST_PROGRAM:
        for (int i = 0; i < node->data.program.count; i++)
            freeAST(node->data.program.statements[i]);
        free(node->data.program.statements);
        break;

    case AST_TYPE:
        switch (node->data.type.typeKind)
        {
        case AST_TYPE_ARRAY:
            freeAST(node->data.type.elementType);
            break;
        case AST_TYPE_TUPLE:
            for (int i = 0; i < node->data.type.tuple.elementCount; i++)
                freeAST(node->data.type.tuple.elementTypes[i]);
            free(node->data.type.tuple.elementTypes);
            break;
        case AST_TYPE_STRUCT:
            free(node->data.type.structType.name);
            for (int i = 0; i < node->data.type.structType.fieldCount; i++)
                freeAST(node->data.type.structType.fields[i]);
            free(node->data.type.structType.fields);
            break;
        default:
            // Nothing additional to free
            break;
        }
        break;

    case AST_ARRAY_LITERAL:
        for (int i = 0; i < node->data.arrayLiteral.elementCount; i++)
            freeAST(node->data.arrayLiteral.elements[i]);
        free(node->data.arrayLiteral.elements);
        break;

    case AST_WHILE:
        freeAST(node->data.whileStmt.condition);
        freeAST(node->data.whileStmt.body);
        break;

    case AST_FOR:
        freeAST(node->data.forStmt.init);
        freeAST(node->data.forStmt.condition);
        freeAST(node->data.forStmt.increment);
        freeAST(node->data.forStmt.body);
        break;

    case AST_FUNCTION_CALL:
        freeAST(node->data.call.callee);
        for (int i = 0; i < node->data.call.argCount; i++)
            freeAST(node->data.call.arguments[i]);
        free(node->data.call.arguments);
        break;

    default:
        // Unknown node type — optionally handle/log here
        break;
    }

    free(node);
}

void printAST(ASTNode *node, int indent)
{
    if (!node)
        return;

    for (int i = 0; i < indent; i++)
        printf("  ");

    switch (node->type)
    {
    case AST_PRINT_STATEMENT:
        printf("PrintStmt:\n");
        if (node->data.printStmt.expr == NULL) {
            for (int i = 0; i < indent + 1; i++)
                printf("  ");
            printf("<empty expr>\n");
        } else {
            printAST(node->data.printStmt.expr, indent + 1);
        }
        break;

    case AST_NUMBER:
        printf("Number: %d\n", node->data.number);
        break;

    case AST_STRING:
        printf("String: \"%s\"\n", node->data.string);
        break;

    case AST_IDENTIFIER:
        printf("Identifier: %s\n", node->data.identifier);
        break;

    case AST_BINARY_EXPR:
        printf("BinaryOp: %s\n", node->data.binary.op->lexeme);
        printAST(node->data.binary.left, indent + 1);
        printAST(node->data.binary.right, indent + 1);
        break;

    case AST_VAR_DECL:
        printf("VarDecl: %s\n", node->data.varDecl.varName);
        if (node->data.varDecl.varType)
        {
            for (int i = 0; i < indent + 1; i++)
                printf("  ");
            printf("TypeAnnotation:\n");
            printAST(node->data.varDecl.varType, indent + 2);
        }
        if (node->data.varDecl.initializer)
        {
            for (int i = 0; i < indent + 1; i++)
                printf("  ");
            printf("Initializer:\n");
            printAST(node->data.varDecl.initializer, indent + 2);
        }
        break;

    case AST_RETURN:
        printf("Return:\n");
        printAST(node->data.returnStmt.expr, indent + 1);
        break;

    case AST_FUNCTION:
        printf("Function: %s\n", node->data.function.name);
        for (int i = 0; i < node->data.function.paramCount; i++)
        {
            for (int j = 0; j < indent + 1; j++)
                printf("  ");
            printf("Param %d:\n", i);
            printAST(node->data.function.params[i], indent + 2);
        }
        if (node->data.function.returnType)
        {
            for (int j = 0; j < indent + 1; j++)
                printf("  ");
            printf("ReturnType:\n");
            printAST(node->data.function.returnType, indent + 2);
        }
        printAST(node->data.function.body, indent + 1);
        break;

    case AST_FUNCTION_CALL:
        printf("FunctionCall:\n");
        for (int j = 0; j < indent + 1; j++)
            printf("  ");
        printf("Callee:\n");
        printAST(node->data.call.callee, indent + 2);
        for (int i = 0; i < node->data.call.argCount; i++)
        {
            for (int j = 0; j < indent + 1; j++)
                printf("  ");
            printf("Arg %d:\n", i);
            printAST(node->data.call.arguments[i], indent + 2);
        }
        break;

    case AST_PROGRAM:
        printf("Program (%d stmts):\n", node->data.program.count);
        for (int i = 0; i < node->data.program.count; i++)
            printAST(node->data.program.statements[i], indent + 1);
        break;

    case AST_EXPR_STMT:
            printf("ExprStmt:\n");
            printAST(node->data.ExprStmt.expr, indent + 2);
            break;
    case AST_IF:
        printf("IfStmt:\n");
        for (int i = 0; i < indent + 1; i++) printf("  ");
        printf("Condition:\n");
        printAST(node->data.ifStmt.condition, indent + 2);
        for (int i = 0; i < indent + 1; i++) printf("  ");
        printf("Then:\n");
        printAST(node->data.ifStmt.thenBranch, indent + 2);
        if (node->data.ifStmt.elseBranch)
        {
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Else:\n");
            printAST(node->data.ifStmt.elseBranch, indent + 2);
        }
        break;

    case AST_WHILE:
        printf("WhileStmt:\n");
        for (int i = 0; i < indent + 1; i++) printf("  ");
        printf("Condition:\n");
        printAST(node->data.whileStmt.condition, indent + 2);
        for (int i = 0; i < indent + 1; i++) printf("  ");
        printf("Body:\n");
        printAST(node->data.whileStmt.body, indent + 2);
        break;

    case AST_FOR:
        printf("ForStmt:\n");
        for (int i = 0; i < indent + 1; i++) printf("  ");
        printf("Init:\n");
        printAST(node->data.forStmt.init, indent + 2);
        for (int i = 0; i < indent + 1; i++) printf("  ");
        printf("Condition:\n");
        printAST(node->data.forStmt.condition, indent + 2);
        for (int i = 0; i < indent + 1; i++) printf("  ");
        printf("Increment:\n");
        printAST(node->data.forStmt.increment, indent + 2);
        for (int i = 0; i < indent + 1; i++) printf("  ");
        printf("Body:\n");
        printAST(node->data.forStmt.body, indent + 2);
        break;

    case AST_ARRAY_LITERAL:
        printf("ArrayLiteral (%d elements):\n", node->data.arrayLiteral.elementCount);
        for (int i = 0; i < node->data.arrayLiteral.elementCount; i++) {
            for (int j = 0; j < indent + 1; j++) printf("  ");
            printf("Element %d:\n", i);
            printAST(node->data.arrayLiteral.elements[i], indent + 2);
        }
        break;

    case AST_TYPE_TUPLE:
        printf("Tuple (%d elements):\n", node->data.type.tuple.elementCount);
        for (int i = 0; i < node->data.type.tuple.elementCount; i++)
            printAST(node->data.type.tuple.elementTypes[i], indent + 1);
        break;

    case AST_TYPE:
        printf("Type: ");
        switch (node->data.type.typeKind)
        {
        case AST_TYPE_INT:
            printf("int\n");
            break;
        case AST_TYPE_FLOAT:
            printf("float\n");
            break;
        case AST_TYPE_BOOL:
            printf("bool\n");
            break;
        case AST_TYPE_STRING:
            printf("string\n");
            break;
        case AST_TYPE_VOID:
            printf("void\n");
            break;
        case AST_TYPE_ARRAY:
            printf("array of:\n");
            printAST(node->data.type.elementType, indent + 1);
            break;
        case AST_TYPE_STRUCT:
            printf("struct %s\n", node->data.type.structType.name);
            for (int i = 0; i < node->data.type.structType.fieldCount; i++)
                printAST(node->data.type.structType.fields[i], indent + 1);
            break;
        default:
            printf("Unknown typeKind: %d\n", node->data.type.typeKind);
            break;
        }
        break;

    default:
        printf("Unknown node type: %d\n", node->type);
        break;
    }
}
