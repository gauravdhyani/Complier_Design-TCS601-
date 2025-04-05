#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global current token pointer (we assume tokens are produced one at a time)
static Token* current_token = NULL;

// Utility: Advance to the next token.
static void advance_token() {
    if(current_token) {
        free(current_token->lexeme);
        free(current_token);
    }
    current_token = get_next_token();
}

// Utility: Check that the current token is of expected type; if yes, advance.
static void expect_token(TokenType type, const char* errMsg) {
    if (current_token->type != type) {
        fprintf(stderr, "Parse error at line %d, col %d: %s (found '%s')\n",
                current_token->line, current_token->col, errMsg, current_token->lexeme);
        exit(EXIT_FAILURE);
    }
    advance_token();
}

// Create a new AST node.
static ASTNode* create_ast_node(ASTNodeType type, const char* value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = type;
    if(value) {
        node->value = strdup(value);
    } else {
        node->value = NULL;
    }
    node->left = node->right = node->next = NULL;
    return node;
}

// Forward declarations
ASTNode* parse_expression();

// --- Parsing functions ---

// parse_program: Parse a list of statements.
ASTNode* parse_program() {
    ASTNode* program = create_ast_node(AST_PROGRAM, NULL);
    ASTNode* last = NULL;
    
    advance_token();  // Prime the token stream.
    
    while (current_token->type != TOKEN_EOF) {
        ASTNode* stmt = parse_statement();
        if (!program->left) {
            program->left = stmt;
        } else {
            last->next = stmt;
        }
        last = stmt;
    }
    return program;
}

// parse_statement: Decide which statement to parse.
ASTNode* parse_statement() {
    // Depending on the token, decide what kind of statement it is.
    if (current_token->type == TOKEN_KEYWORD_FN) {
        return parse_function_decl();
    } else if (current_token->type == TOKEN_KEYWORD_VAR) {
        return parse_var_decl();
    } else if (current_token->type == TOKEN_KEYWORD_RETURN) {
        return parse_return_stmt();
    } else {
        // Otherwise, treat as an expression statement.
        ASTNode* expr = parse_expression();
        // Expect a semicolon at the end.
        expect_token(TOKEN_DELIM_SEMICOLON, "Expected ';' after expression");
        return expr;
    }
}

// parse_function_decl: Parse a function declaration.
ASTNode* parse_function_decl() {
    // Grammar: fn <identifier> ( <parameter_list> ) -> <datatype> { <block> }
    expect_token(TOKEN_KEYWORD_FN, "Expected 'fn'");
    
    // Function name:
    ASTNode* func = create_ast_node(AST_FUNCTION_DECL, current_token->lexeme);
    expect_token(TOKEN_IDENTIFIER, "Expected function name");
    
    expect_token(TOKEN_DELIM_OPEN_PAREN, "Expected '(' after function name");
    // For simplicity, we’re skipping full parameter list parsing.
    // You could extend this function to parse parameters.
    expect_token(TOKEN_DELIM_CLOSE_PAREN, "Expected ')' after parameter list");
    
    expect_token(TOKEN_ARROW, "Expected '->' for return type");

    // Check if the current token is one of the acceptable return type tokens.
    if (current_token->type == TOKEN_KEYWORD_INT ||
        current_token->type == TOKEN_KEYWORD_FLOAT ||
        current_token->type == TOKEN_KEYWORD_BOOL ||
        current_token->type == TOKEN_KEYWORD_STRING ||
        current_token->type == TOKEN_KEYWORD_VOID) {
        ASTNode* retType = create_ast_node(AST_IDENTIFIER, current_token->lexeme);
        advance_token();
        func->right = retType; // Attach return type node to function node.
    } else {
        fprintf(stderr, "Parse error at line %d, col %d: Expected return type identifier (found '%s')\n",
                current_token->line, current_token->col, current_token->lexeme);
        exit(EXIT_FAILURE);
    }

    
    expect_token(TOKEN_DELIM_OPEN_BRACE, "Expected '{' to begin function body");
    // Parse the function body as a list of statements.
    ASTNode* body = parse_program();
    func->left = body;  // Attach body as left child.
    expect_token(TOKEN_DELIM_CLOSE_BRACE, "Expected '}' to close function body");
    
    return func;
}

// parse_var_decl: Parse a variable declaration.
ASTNode* parse_var_decl() {
    // Grammar: var <identifier> : <datatype> = <expression> ;
    expect_token(TOKEN_KEYWORD_VAR, "Expected 'var'");
    ASTNode* varDecl = create_ast_node(AST_VAR_DECL, current_token->lexeme);
    expect_token(TOKEN_IDENTIFIER, "Expected variable name");
    
    expect_token(TOKEN_DELIM_COLON, "Expected ':' after variable name");

    // Accept a type token: it might be a keyword (e.g. TOKEN_KEYWORD_INT, TOKEN_KEYWORD_FLOAT, etc.)
    if (current_token->type == TOKEN_KEYWORD_INT ||
        current_token->type == TOKEN_KEYWORD_FLOAT ||
        current_token->type == TOKEN_KEYWORD_BOOL ||
        current_token->type == TOKEN_KEYWORD_STRING ||
        current_token->type == TOKEN_KEYWORD_VOID) {
        ASTNode* typeNode = create_ast_node(AST_IDENTIFIER, current_token->lexeme);
        advance_token();
        varDecl->left = typeNode;
    } else {
        fprintf(stderr, "Parse error at line %d, col %d: Expected type identifier (found '%s')\n",
                current_token->line, current_token->col, current_token->lexeme);
        exit(EXIT_FAILURE);
    }


    // ASTNode* typeNode = create_ast_node(AST_IDENTIFIER, current_token->lexeme);
    // expect_token(TOKEN_IDENTIFIER, "Expected type identifier");
    // varDecl->left = typeNode;
    
    expect_token(TOKEN_OPERATOR_ASSIGN, "Expected '=' in variable declaration");
    ASTNode* expr = parse_expression();
    varDecl->right = expr;

    printf("Debug: current token is '%s'\n", current_token->lexeme);
    
    expect_token(TOKEN_DELIM_SEMICOLON, "Expected ';' after variable declaration");
    return varDecl;
}

// parse_return_stmt: Parse a return statement.
ASTNode* parse_return_stmt() {
    expect_token(TOKEN_KEYWORD_RETURN, "Expected 'return'");
    ASTNode* retStmt = create_ast_node(AST_RETURN_STMT, "return");
    ASTNode* expr = parse_expression();
    retStmt->left = expr;
    expect_token(TOKEN_DELIM_SEMICOLON, "Expected ';' after return statement");
    return retStmt;
}

// parse_expression: For simplicity, parse a binary expression with only '+' operator.
ASTNode* parse_expression() {
    // Start with a primary expression.
    ASTNode* left = NULL;
    // Here, we assume an expression starts with an identifier or a number.
    if (current_token->type == TOKEN_IDENTIFIER) {
        left = create_ast_node(AST_IDENTIFIER, current_token->lexeme);
        advance_token();
    } else if (current_token->type == TOKEN_NUMBER) {
        left = create_ast_node(AST_LITERAL, current_token->lexeme);
        advance_token();
    } else {
        fprintf(stderr, "Parse error: Unexpected token '%s' in expression\n", current_token->lexeme);
        exit(EXIT_FAILURE);
    }
    
    // Handle binary operators (only '+' for this example)
    while (current_token->type == TOKEN_OPERATOR_PLUS) {
        Token* op = current_token;
        advance_token();
        ASTNode* right = NULL;
        if (current_token->type == TOKEN_IDENTIFIER) {
            right = create_ast_node(AST_IDENTIFIER, current_token->lexeme);
            advance_token();
        } else if (current_token->type == TOKEN_NUMBER) {
            right = create_ast_node(AST_LITERAL, current_token->lexeme);
            advance_token();
        } else {
            fprintf(stderr, "Parse error: Expected identifier or number after '+'\n");
            exit(EXIT_FAILURE);
        }
        ASTNode* binary = create_ast_node(AST_BINARY_EXPR, op->lexeme);
        binary->left = left;
        binary->right = right;
        left = binary;
    }
    
    return left;
}

// Free AST recursively.
void free_ast(ASTNode* node) {
    if (!node)
        return;
    free_ast(node->left);
    free_ast(node->right);
    free_ast(node->next);
    if (node->value)
        free(node->value);
    free(node);
}

// (Optional) Function to print the AST for debugging.
void print_ast(ASTNode* node, int indent) {
    if (!node) return;
    for (int i=0; i<indent; i++) printf("  ");
    printf("%d: ", node->type);
    if (node->value)
        printf("%s\n", node->value);
    else
        printf("\n");
    print_ast(node->left, indent+1);
    print_ast(node->right, indent+1);
    print_ast(node->next, indent);
}

/*
// --- Main function to test the parser ---
int main() {
    // Example source code for testing (this should match your mini language)
    char* source = 
        "fn main() -> Int {"
        "  var x: Int = 10;"
        "  var y: Int = 20;"
        "  var z: Int = x + y;"
        "  return z;"
        "}";
    
    // Initialize lexer with the source code.
    initlexer(source);
    
    // Parse the program.
    ASTNode* ast = parse_program();
    
    printf("Parsing successful!\n");
    // Print the AST for debugging.
    print_ast(ast, 0);
    
    free_ast(ast);
    
    return 0;
}
*/