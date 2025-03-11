// Token separators: space,\t,\n,\r\n
// Comments will be **
// Multiline comments: *- code -*
// Keywords: fn-function declaration,if,else,var,return,import,loop(while),forloop(for)
// Identifiers: An identifier begins with a letter (A–Z or a–z) or an underscore (_), followed by any combination of letters, digits (0–9), or underscores.
// Literals: A sequence of one or more digits.
// Float: One or more digits, a decimal point, then one or more digits. (Optionally, you could add exponent notation later.)
// String: A sequence of characters enclosed in double quotes. Supports escape sequences
// Operators:+,-,*,/,%,=, ==, !=, <, <=, >, >=,&&, ||, !
// Return type of function : Arrow: ->
// Delimeters:(),{},[],”,”,:,;,.
// Datatype:Int,Float,Bool,String,Void,
// Arrays:var arr:[Int]=[1,2,3,4],var p:(Int,Int)=(10,20);
// String array: var user: { String: String } = { "name": "Alice", "role": "admin" };
// Structure:struct p{ name:String,age:Int}
//based on this provide code completions


#ifndef LEXER_H
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>


/* 
 * TokenType: Enum of all token types that our lexer can produce.
 */

typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_KEYWORD_FN,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_VAR,
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_IMPORT,
    TOKEN_KEYWORD_LOOP,
    TOKEN_KEYWORD_FORLOOP,
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_FLOAT,
    TOKEN_KEYWORD_BOOL,
    TOKEN_OPERATOR_PLUS,
    TOKEN_OPERATOR_MINUS,
    TOKEN_OPERATOR_MUL,
    TOKEN_OPERATOR_DIV,
    TOKEN_OPERATOR_MOD,
    TOKEN_OPERATOR_ASSIGN,
    TOKEN_OPERATOR_EQ,
    TOKEN_OPERATOR_NEQ,
    TOKEN_OPERATOR_LT,
    TOKEN_OPERATOR_LTE,
    TOKEN_OPERATOR_GT,
    TOKEN_OPERATOR_GTE,
    TOKEN_OPERATOR_AND,
    TOKEN_OPERATOR_OR,
    TOKEN_OPERATOR_NOT,
    TOKEN_DELIM_OPEN_PAREN,
    TOKEN_DELIM_CLOSE_PAREN,
    TOKEN_DELIM_OPEN_BRACE,
    TOKEN_DELIM_CLOSE_BRACE,
    TOKEN_DELIM_OPEN_SQUARE,
    TOKEN_DELIM_CLOSE_SQUARE,
    TOKEN_DELIM_COMMA,
    TOKEN_DELIM_COLON,
    TOKEN_DELIM_SEMICOLON,
    TOKEN_DELIM_DOT,
    TOKEN_ARROW,
    TOKEN_KEYWORD_VOID,
    TOKEN_KEYWORD_STRUCT,
    TOKEN_KEYWORD_STRING,
    TOKEN_KEYWORD_TRUE,
    TOKEN_KEYWORD_FALSE,
    TOKEN_KEYWORD_NULL
} TokenType; 

/* 
 * Token: A struct that represents a token produced by the lexer.
 * type: The type of the token.
 * lexeme: The string that the token represents.
 * line: The line number in the source code where the token was found.
 * col: The column number in the source code where the token was found.
 */

typedef struct {
    TokenType type;
    char* lexeme;
    int line;
    int col;
} Token;

#define LEXER_H
#endif