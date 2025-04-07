#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include your headers
#include "lexer.h"
#include "parser.h"
#include "semanticanalyser.h"
#include "executionengine.h"

int main() {
    char *source_code =
        "function factorial(n: int): int {\n"
        "  if n <= 1 {\n"
        "    return 1;\n"
        "  } else {\n"
        "    return n * factorial(n - 1);\n"
        "  }\n"
        "}\n"
        "var result: int = factorial(5);";

    initlexer(source_code);
    ASTNode *ast = parse_program();

    if (!ast) {
        fprintf(stderr, "Parsing failed.\n");
        return 1;
    }

    traverse(ast);
    execute(ast);

    return 0;
}
