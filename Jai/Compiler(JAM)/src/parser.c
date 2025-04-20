#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Forward declarations ---
ASTNode* parseProgram(Parser* p);
static ASTNode* parseFunction(Parser* p);
static ASTNode* parseStatement(Parser* p);
static ASTNode* parseVarDecl(Parser* p);
static ASTNode* parseExpression(Parser* p);
static ASTNode* parseTerm(Parser* p);
static ASTNode* parsePrimary(Parser* p);

// --- Lexer lookahead helpers ---
static Token* peek(Parser* p) {
    return (p->current < p->tokenCount
            ? p->tokens[p->current]
            : NULL);
}
static Token* advance(Parser* p) {
    return (p->current < p->tokenCount
            ? p->tokens[p->current++]
            : NULL);
}
static int match(Parser* p, TokenType t) {
    Token* tok = peek(p);
    if (tok && tok->type == t) {
        advance(p);
        return 1;
    }
    return 0;
}
static void consume(Parser* p, TokenType t, const char* msg) {
    if (!match(p, t)) {
        Token* tok = peek(p);
        fprintf(stderr,
                "Parse error at line %d col %d: %s (got '%s')\n",
                tok ? tok->line : -1,
                tok ? tok->col  : -1,
                msg,
                tok ? tok->lexeme : "EOF");
        exit(1);
    }
}

// --- AST node constructors ---
static ASTNode* makeNode(ASTNodeType type) {
    ASTNode* n = malloc(sizeof(ASTNode));
    n->type = type;
    memset(&n->data, 0, sizeof(n->data));
    return n;
}
static ASTNode* literalNode(Token* tok) {
    if (tok->type == TOKEN_STRING) {
        ASTNode* n = makeNode(AST_STRING);
        n->data.string = strdup(tok->lexeme);
        return n;
    }
    ASTNode* n = makeNode(AST_NUMBER);
    n->data.number = atoi(tok->lexeme);
    return n;
}
static ASTNode* binaryNode(ASTNode* left, Token* op, ASTNode* right) {
    ASTNode* n = makeNode(AST_BINARY_EXPR);
    n->data.binary.left     = left;
    n->data.binary.operator = op;
    n->data.binary.right    = right;
    return n;
}

// --- Parsing functions ---

// primary ::= NUMBER | STRING | IDENTIFIER | '(' expression ')'
static ASTNode* parsePrimary(Parser* p) {
    Token* t = peek(p);
    if (!t) return NULL;
    if (t->type == TOKEN_NUMBER || t->type == TOKEN_STRING) {
        advance(p);
        return literalNode(t);
    }
    if (t->type == TOKEN_IDENTIFIER) {
        ASTNode* n = makeNode(AST_IDENTIFIER);
        n->data.identifier = strdup(t->lexeme);
        advance(p);
        return n;
    }
    if (match(p, TOKEN_DELIM_OPEN_PAREN)) {
        ASTNode* e = parseExpression(p);
        consume(p, TOKEN_DELIM_CLOSE_PAREN, "Expected ')'");
        return e;
    }
    fprintf(stderr, "Unexpected '%s' in primary\n", t->lexeme);
    exit(1);
}

// term ::= primary ( (*|/|%) primary )*
static ASTNode* parseTerm(Parser* p) {
    ASTNode* node = parsePrimary(p);
    while (1) {
        Token* t = peek(p);
        if (t && (t->type == TOKEN_OPERATOR_MUL ||
                  t->type == TOKEN_OPERATOR_DIV ||
                  t->type == TOKEN_OPERATOR_MOD)) {
            advance(p);
            node = binaryNode(node, t, parsePrimary(p));
        } else break;
    }
    return node;
}

// expression ::= term ( (+|-) term )*
static ASTNode* parseExpression(Parser* p) {
    ASTNode* node = parseTerm(p);
    while (1) {
        Token* t = peek(p);
        if (t && (t->type == TOKEN_OPERATOR_PLUS ||
                  t->type == TOKEN_OPERATOR_MINUS)) {
            advance(p);
            node = binaryNode(node, t, parseTerm(p));
        } else break;
    }
    return node;
}

// varDecl ::= 'var' IDENT [ '=' expression ] ';'
static ASTNode* parseVarDecl(Parser* p) {
    advance(p); // consume 'var'
    Token* id = peek(p);
    consume(p, TOKEN_IDENTIFIER, "Expected variable name");
    ASTNode* n = makeNode(AST_VAR_DECL);
    n->data.varDecl.varName = strdup(id->lexeme);
    if (match(p, TOKEN_OPERATOR_ASSIGN)) {
        n->data.varDecl.initializer = parseExpression(p);
    }
    consume(p, TOKEN_DELIM_SEMICOLON, "Expected ';' after var declaration");
    return n;
}

// statement ::= 'return' expression ';'
//             | varDecl
//             | expression ';'
static ASTNode* parseStatement(Parser* p) {
    Token* t = peek(p);
    if (!t) return NULL;
    if (t->type == TOKEN_KEYWORD_RETURN) {
        advance(p);
        ASTNode* n = makeNode(AST_RETURN);
        n->data.returnStmt.expr = parseExpression(p);
        consume(p, TOKEN_DELIM_SEMICOLON, "Expected ';' after return");
        return n;
    }
    if (t->type == TOKEN_KEYWORD_VAR) {
        return parseVarDecl(p);
    }
    ASTNode* expr = parseExpression(p);
    consume(p, TOKEN_DELIM_SEMICOLON, "Expected ';' after expression");
    return expr;
}

// function ::= 'fn' IDENT '(' ')' BLOCK
static ASTNode* parseFunction(Parser* p) {
    advance(p); // consume 'fn'
    Token* name = peek(p);
    consume(p, TOKEN_IDENTIFIER, "Expected function name");
    ASTNode* fn = makeNode(AST_FUNCTION);
    fn->data.function.name = strdup(name->lexeme);

    consume(p, TOKEN_DELIM_OPEN_PAREN, "Expected '(' after function name");
    consume(p, TOKEN_DELIM_CLOSE_PAREN, "Expected ')' (no params)");

    consume(p, TOKEN_DELIM_OPEN_BRACE, "Expected '{' to begin function body");
    // reuse program struct to hold function body
    fn->data.function.body = makeNode(AST_PROGRAM);
    fn->data.function.body->data.program.statements = NULL;
    fn->data.function.body->data.program.count = 0;

    while (!match(p, TOKEN_DELIM_CLOSE_BRACE)) {
        ASTNode* stmt = parseStatement(p);
        // append
        int c = fn->data.function.body->data.program.count++;
        fn->data.function.body->data.program.statements = realloc(
            fn->data.function.body->data.program.statements,
            fn->data.function.body->data.program.count * sizeof(ASTNode*));
        fn->data.function.body->data.program.statements[c] = stmt;
    }
    return fn;
}
void initParser(Parser* p, Token** tokens, int tokenCount) {
    p->tokens     = tokens;
    p->tokenCount = tokenCount;
    p->current    = 0;
}

ASTNode* parseProgram(Parser* p) {
    ASTNode* prog = makeNode(AST_PROGRAM);
    prog->data.program.statements = NULL;
    prog->data.program.count = 0;

    while (peek(p) && peek(p)->type != TOKEN_EOF) {
        ASTNode* node;
        if (peek(p)->type == TOKEN_KEYWORD_FN) {
            node = parseFunction(p);
        } else {
            node = parseStatement(p);
        }
        int c = prog->data.program.count++;
        prog->data.program.statements = realloc(
            prog->data.program.statements,
            prog->data.program.count * sizeof(ASTNode*));
        prog->data.program.statements[c] = node;
    }
    return prog;
}

void freeAST(ASTNode* node) {
    if (!node) return;
    switch (node->type) {
        case AST_STRING:   free(node->data.string); break;
        case AST_IDENTIFIER: free(node->data.identifier); break;
        case AST_BINARY_EXPR:
            freeAST(node->data.binary.left);
            freeAST(node->data.binary.right);
            break;
        case AST_VAR_DECL:
            free(node->data.varDecl.varName);
            freeAST(node->data.varDecl.initializer);
            break;
        case AST_RETURN:
            freeAST(node->data.returnStmt.expr);
            break;
        case AST_FUNCTION:
            free(node->data.function.name);
            freeAST(node->data.function.body);
            break;
        case AST_PROGRAM:
            for (int i = 0; i < node->data.program.count; i++)
                freeAST(node->data.program.statements[i]);
            free(node->data.program.statements);
            break;
        case AST_NUMBER:
            break;
    }
    free(node);
}

void printAST(ASTNode* node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; i++) printf("  ");
    switch (node->type) {
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
            printf("BinaryOp: %s\n", node->data.binary.operator->lexeme);
            printAST(node->data.binary.left, indent+1);
            printAST(node->data.binary.right, indent+1);
            break;
        case AST_VAR_DECL:
            printf("VarDecl: %s\n", node->data.varDecl.varName);
            if (node->data.varDecl.initializer)
                printAST(node->data.varDecl.initializer, indent+1);
            break;
        case AST_RETURN:
            printf("Return:\n");
            printAST(node->data.returnStmt.expr, indent+1);
            break;
        case AST_FUNCTION:
            printf("Function: %s\n", node->data.function.name);
            printAST(node->data.function.body, indent+1);
            break;
        case AST_PROGRAM:
            printf("Program (%d stmts):\n", node->data.program.count);
            for (int i = 0; i < node->data.program.count; i++)
                printAST(node->data.program.statements[i], indent+1);
            break;
    }
}
