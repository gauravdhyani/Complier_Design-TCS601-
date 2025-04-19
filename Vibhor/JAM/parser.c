#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ======================
// Parser Utilities
// ======================

static Token* peek(Parser* parser) {
    if (parser->current >= parser->tokenCount) return NULL;
    return parser->tokens[parser->current];
}

Token* parser_advance(Parser* parser) {
    if (parser->current >= parser->tokenCount) return NULL;
    return parser->tokens[parser->current++];
}

int match(Parser* parser, TokenType type) {
    Token* t = peek(parser);
    if (t && t->type == type) {
        parser_advance(parser);
        return 1;
    }
    return 0;
}

static void consume(Parser* parser, TokenType type, const char* error) {
    if (!match(parser, type)) {
        Token* t = peek(parser);
        fprintf(stderr, "Error at line %d, col %d: %s. Found %s\n",
                t->line, t->col, error, t->lexeme);
        exit(1);
    }
}

ASTNode* create_node(ASTNodeType type) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = type;
    memset(&node->data, 0, sizeof(node->data));
    return node;
}

ASTNode* create_literal_node(Token* token) {
    ASTNode* node = create_node(NODE_TYPE_LITERAL);
    if (token->type == TOKEN_NUMBER) {
        node->data.literal.num = atoi(token->lexeme);  // Store number
        node->data.literal.str = NULL;  // Explicitly set string to NULL
    } else if (token->type == TOKEN_STRING) {
        node->data.literal.str = strdup(token->lexeme);  // Store string
        node->data.literal.num = 0;  // Explicitly set number to 0
    }
    return node;
}

ASTNode* create_binary_node(ASTNode* left, Token* op, ASTNode* right) {
    ASTNode* node = create_node(NODE_TYPE_BINARY_OP);
    node->data.binary.left = left;
    node->data.binary.op = op;
    node->data.binary.right = right;
    return node;
}

// ======================
// Parsing Functions
// ======================

ASTNode* parse_expression(Parser* parser);
ASTNode* parse_term(Parser* parser);
ASTNode* parse_factor(Parser* parser);
ASTNode* parse_primary(Parser* parser);

ASTNode* parse_primary(Parser* parser) {
    Token* t = peek(parser);
    if (!t) return NULL;

    switch(t->type) {
        case TOKEN_NUMBER:
        case TOKEN_STRING: {
            parser_advance(parser);
            return create_literal_node(t);
        }
        case TOKEN_IDENTIFIER: {
            ASTNode* node = create_node(NODE_TYPE_IDENTIFIER);
            node->data.str = strdup(t->lexeme);
            parser_advance(parser);
            return node;
        }
        case TOKEN_DELIM_OPEN_PAREN: {
            parser_advance(parser);
            ASTNode* expr = parse_expression(parser);
            consume(parser, TOKEN_DELIM_CLOSE_PAREN, "Expected ')' after expression");
            return expr;
        }
        default:
            fprintf(stderr, "Unexpected token in primary expression: %s\n", t->lexeme);
            exit(1);
    }
}

ASTNode* parse_factor(Parser* parser) {
    ASTNode* node = parse_primary(parser);
    
    // Handle array access
    while (match(parser, TOKEN_DELIM_OPEN_SQUARE)) {
        ASTNode* index = parse_expression(parser);
        consume(parser, TOKEN_DELIM_CLOSE_SQUARE, "Expected ']' after array index");
        Token* op = create_token(TOKEN_DELIM_OPEN_SQUARE, "[]", 0, 0);
        node = create_binary_node(node, op, index);
    }
    
    return node;
}

ASTNode* parse_term(Parser* parser) {
    ASTNode* node = parse_factor(parser);
    
    while (1) {
        Token* t = peek(parser);
        if (!t) break;
        
        if (t->type == TOKEN_OPERATOR_MUL || 
            t->type == TOKEN_OPERATOR_DIV ||
            t->type == TOKEN_OPERATOR_MOD) {
            parser_advance(parser);
            ASTNode* right = parse_factor(parser);
            node = create_binary_node(node, t, right);
        } else {
            break;
        }
    }
    
    return node;
}

ASTNode* parse_expression(Parser* parser) {
    ASTNode* node = parse_term(parser);
    if (!node) {
        fprintf(stderr, "Expected expression\n");
        exit(1);
    }

    while (1) {
        Token* t = peek(parser);
        if (!t) break;
        
        if (t->type == TOKEN_OPERATOR_PLUS || 
            t->type == TOKEN_OPERATOR_MINUS) {
            parser_advance(parser);
            ASTNode* right = parse_term(parser);
            node = create_binary_node(node, t, right);
        } else {
            break;
        }
    }
    
    return node;
}

ASTNode* parse_assignment(Parser* parser) {
    Token* id = peek(parser);
    if (id->type != TOKEN_IDENTIFIER) {
        return parse_expression(parser);
    }
    
    ASTNode* left = parse_primary(parser);
    if (!match(parser, TOKEN_OPERATOR_ASSIGN)) {
        return left;
    }
    
    ASTNode* node = create_node(NODE_TYPE_ASSIGNMENT);
    node->data.binary.left = left;
    node->data.binary.right = parse_expression(parser);
    return node;
}

ASTNode* parse_statement(Parser* parser) {
    Token* t = peek(parser);
    if (!t) return NULL;

    switch(t->type) {
        case TOKEN_KEYWORD_RETURN: {
            parser_advance(parser);
            ASTNode* node = create_node(NODE_TYPE_RETURN);
            node->data.binary.right = parse_expression(parser);
            consume(parser, TOKEN_DELIM_SEMICOLON, "Expected ';' after return");
            return node;
        }
        case TOKEN_KEYWORD_VAR: {
            return parse_declaration(parser);
        }
        default: {
            ASTNode* expr = parse_assignment(parser);
            if (match(parser, TOKEN_DELIM_SEMICOLON)) {
                return expr;
            }
            fprintf(stderr, "Expected ';' after statement\n");
            exit(1);
        }
    }
}

ASTNode* parse_block(Parser* parser) {
    consume(parser, TOKEN_DELIM_OPEN_BRACE, "Expected '{'");
    ASTNode* block = create_node(NODE_TYPE_BLOCK);
    
    // Allocate space for statements
    ASTNode** statements = malloc(MAX_STATEMENTS * sizeof(ASTNode*));
    int stmt_count = 0;
    
    // Parse all statements until '}'
    while (!match(parser, TOKEN_DELIM_CLOSE_BRACE)) {
        if (stmt_count >= MAX_STATEMENTS) {
            fprintf(stderr, "Too many statements in block\n");
            exit(1);
        }
        
        ASTNode* stmt = parse_statement(parser);
        statements[stmt_count++] = stmt;
        
        // Debug print
        printf("Parsed statement %d in block\n", stmt_count);
    }
    
    // Store statements in block node
    block->data.block.statements = statements;
    block->data.block.statement_count = stmt_count;
    
    return block;
}

ASTNode* parse_function(Parser* parser) {
    consume(parser, TOKEN_KEYWORD_FN, "Expected 'fn'");
    
    Token* name = peek(parser);
    consume(parser, TOKEN_IDENTIFIER, "Expected function name");
    
    ASTNode* func_node = create_node(NODE_TYPE_FUNCTION);
    // func_node->data.str = strdup(name->lexeme);
    func_node->data.function.name = strdup(name->lexeme);
    
    consume(parser, TOKEN_DELIM_OPEN_PAREN, "Expected '('");
    // TODO: Parse parameters
    consume(parser, TOKEN_DELIM_CLOSE_PAREN, "Expected ')'");
    
    func_node->data.function.body = parse_block(parser);
    return func_node;
}

// ======================
// Public Interface
// ======================

Parser* init_parser(Token** tokens, int tokenCount) {
    Parser* parser = malloc(sizeof(Parser));
    parser->tokens = tokens;
    parser->tokenCount = tokenCount;
    parser->current = 0;
    return parser;
}

ASTNode* parse_declaration(Parser *parser) {
    consume(parser, TOKEN_KEYWORD_VAR, "Expected 'var'");
    ASTNode* node = create_node(NODE_TYPE_DECLARATION);
    
    // Store variable name
    Token* id = peek(parser);

    if (!id || id->type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected identifier after 'var'\n");
        exit(1);
    }

    consume(parser, TOKEN_IDENTIFIER, "Expected variable name");
    node->data.declaration.name = strdup(id->lexeme);  // Store name in declaration struct
    
    if (match(parser, TOKEN_OPERATOR_ASSIGN)) {
        node->data.declaration.value = parse_expression(parser);
    }
    else {
        node->data.declaration.value = NULL; // Explicitly mark as uninitialized
    }
    
    consume(parser, TOKEN_DELIM_SEMICOLON, "Expected ';' after declaration");
    return node;
}

ASTNode* parse(Parser* parser) {
    ASTNode* program = create_node(NODE_TYPE_BLOCK);
    
    // Allocate space for statements
    program->data.block.statements = malloc(MAX_STATEMENTS * sizeof(ASTNode*));
    program->data.block.statement_count = 0;
    
    while (peek(parser) && peek(parser)->type != TOKEN_EOF) {
        if (program->data.block.statement_count >= MAX_STATEMENTS) {
            fprintf(stderr, "Too many top-level declarations\n");
            exit(1);
        }
        
        if (peek(parser)->type == TOKEN_KEYWORD_FN) {
            program->data.block.statements[program->data.block.statement_count++] = 
                parse_function(parser);
        } else {
            program->data.block.statements[program->data.block.statement_count++] = 
                parse_statement(parser);
        }
    }
    
    return program;
}

void print_block(ASTNode *block, int indent) {
    for (int i=0; i<block->data.block.statement_count; i++) {
        print_ast(block->data.block.statements[i], indent+1);
    }
}

void print_ast(ASTNode* node, int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
    
    switch(node->type) {
        case NODE_TYPE_LITERAL:
            if (node->data.literal.str) {
                printf("String: %s\n", node->data.literal.str);
            } else {
                printf("Number: %d\n", node->data.literal.num);
            }
            break;
        case NODE_TYPE_IDENTIFIER:
            printf("Identifier: %s\n", node->data.str);
            break;
        case NODE_TYPE_BINARY_OP:
            printf("BinaryOp: %s\n", node->data.binary.op->lexeme);
            print_ast(node->data.binary.left, indent+1);
            print_ast(node->data.binary.right, indent+1);
            break;
        case NODE_TYPE_RETURN:
            printf("Return:\n");
            print_ast(node->data.binary.right, indent+1);
            break;
        case NODE_TYPE_FUNCTION:
            printf("Function %s:\n", node->data.function.name);
            print_ast(node->data.function.body, indent+1);
            break;
        case NODE_TYPE_BLOCK:
            printf("Block:\n");
            for (int i = 0; i < node->data.block.statement_count; i++) {
                print_ast(node->data.block.statements[i], indent+1);
            }
            break;
        case NODE_TYPE_DECLARATION:
            printf("Declaration %s:\n", node->data.declaration.name);
            if (node->data.declaration.value) {
                print_ast(node->data.declaration.value, indent+1);
            }
            break;
        default:
            printf("Unknown node type: %d\n", node->type);
    }
}

// int main() {
//     // Initialize with sample source code
//     char* source = "fn main() { var x = 10; return x + 5; }";
//         // char* source = "var x = 5;";
//     // char* source = "fn main() {\n"
//     // "    var arr:[Int]=[1,2,3,4]\n"
//     // "    var y = 20;\n"
//     // "    var z = x + y;\n"
//     // "    return arr[0];\n"
//     // "}\n";
//     initlexer(source);
    
//     // Token collection - array of pointers
//     Token* tokens[100];
//     int token_count = 0;
    
//     // Lexer loop
//     while (token_count < 100) {
//         Token* token = get_next_token();
//         tokens[token_count++] = token;
        
//         // Debug print each token
//         printf("Token %d: %s (Type: %d, Line: %d, Col: %d)\n", 
//                token_count,
//                token->lexeme, 
//                token->type,
//                token->line,
//                token->col);
        
//         if (token->type == TOKEN_EOF) {
//             break;
//         }
//     }
    
//     // Initialize parser
//     Parser* parser = init_parser(tokens, token_count);
    
//     // Parse the tokens
//     ASTNode* ast = parse(parser);
    
//     // Print the AST
//     printf("\nAbstract Syntax Tree:\n");
//     print_ast(ast, 0);
    
//     // Cleanup
//     free_ast(ast);
    
//     // Free tokens
//     for (int i = 0; i < token_count; i++) {
//         free(tokens[i]->lexeme);
//         free(tokens[i]);
//     }
    
//     free(parser);
//     return 0;
// }

ASTNode *parse_program()
{

    Token* tokens[100];
    int token_count = 0;
    
    // Lexer loop
    while (token_count < 100) {
        Token* token = get_next_token();
        tokens[token_count++] = token;
        
        // Debug print each token
        printf("Token %d: %s (Type: %d, Line: %d, Col: %d)\n", 
               token_count,
               token->lexeme, 
               token->type,
               token->line,
               token->col);
        
        if (token->type == TOKEN_EOF) {
            break;
        }
    }

    // Initialize parser
    Parser* parser = init_parser(tokens, token_count);
    
    // Parse the tokens
    ASTNode* ast = parse(parser);

     // Cleanup
     free_ast(ast);
    
     // Free tokens
     for (int i = 0; i < token_count; i++) {
         free(tokens[i]->lexeme);
         free(tokens[i]);
     }
     
    free(parser);

    return ast;
}

// Recursive AST cleanup function
void free_ast(ASTNode* node) {
    if (!node) return;
    
    switch(node->type) {
        case NODE_TYPE_IDENTIFIER:
        case NODE_TYPE_LITERAL:
            if (node->data.literal.str) {
                free(node->data.literal.str);
            }
            break;
            
        case NODE_TYPE_BINARY_OP:
            free_ast(node->data.binary.left);
            free_ast(node->data.binary.right);
            break;
            
        case NODE_TYPE_FUNCTION:
            free(node->data.function.name);
            free_ast(node->data.function.body);
            break;
        
        case NODE_TYPE_DECLARATION:
            free(node->data.declaration.name);
            if (node->data.declaration.value) {
                free_ast(node->data.declaration.value);
            }
            break;
            
        case NODE_TYPE_RETURN:
            free_ast(node->data.binary.right);
            break;
            
        case NODE_TYPE_BLOCK:
            // If you implement statement lists, free them here
            for (int i=0; i<node->data.block.statement_count; i++) {
                free_ast(node->data.block.statements[i]);
            }
            free(node->data.block.statements);
            break;
            
        default:
            break;
    }
    
    free(node);
}