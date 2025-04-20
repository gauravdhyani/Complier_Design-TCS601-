#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char* source =
        "fn main() {\n"
        "    var x = 10;\n"
        "    var y = 20;\n"
        "    var z = x + y;\n"
        "    return z;\n"
        "}\n";

    // 1) Lex
    initlexer((char*)source);

    Token** tokens = NULL;
    int      count = 0, cap = 0;
    //store tokens
    while (1) {
        Token* t = get_next_token();
        if (count == cap) {
            cap = cap ? cap * 2 : 8;
            tokens = realloc(tokens, cap * sizeof(Token*));
        }
        tokens[count++] = t;
        if (t->type == TOKEN_EOF) break;
    }

    // 2) Parse
    Parser parser;
    initParser(&parser, tokens, count);
    ASTNode* ast = parseProgram(&parser);

    // 3) Print
    puts("\n===== AST =====");
    printAST(ast, 0);

    // 4) Cleanup
    freeAST(ast);
    for (int i = 0; i < count; i++) {
        free(tokens[i]->lexeme);
        free(tokens[i]);
    }
    free(tokens);
    return 0;
}
