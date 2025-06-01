#include "lexer.h"
#include "parser.h"
#include "semanticanalyser.h"
#include "executionengine.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char* source = 
    "var arr: [Int] = [1, 2, 3, 4];\n"
    "var i: Int = 0;\n"
    "print(\"loop\");\n"
    "while (i < 10) {\n"
    "    i = i + 1;\n"
    "    print(i);\n"
    "}\n"
    "print(\"Factorial of 5:\");\n"
    "var result: Int = factorial(5);\n"
    "print(result);\n"
    "fn factorial(n: Int) -> Int {\n"
    "    if (n <= 1) {\n"
    "        return 1;\n"
    "    } else {\n"
    "        return n * factorial(n - 1);\n"
    "    }\n"
    "}\n";
    // Lex

    initlexer((char*)source);
    Token** tokens = NULL;
    printf("===== Tokens =====\n");
    int count = 0, cap = 0;
    while (1) {
        Token* t = get_next_token();
        printf("Token(type=%d, lexeme='%s', line=%d, col=%d)\n",(int)t->type, t->lexeme, t->line, t->col);
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
    // Print AST
    puts("\n===== AST =====");
    printAST(ast, 0);

    // Semantic analysis
    enterScope();
    printf("\n===== Semantic Analysis =====\n");
    debugTraverse(ast);
    printf("Semantic analysis completed successfully.\n\n");
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

    return 0;
}
