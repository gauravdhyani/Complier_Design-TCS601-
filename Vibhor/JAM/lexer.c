#include "./lexer.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>

typedef struct {
    char *source;
    int line,pos,col;
}Lexer;

static Lexer lexer;
Token* get_next_token();
Token* create_token(TokenType,char*, int, int);
Token* identifier();
Token* number();
Token* string();
void skip_comments();
void init_lexer(const char* source);
Token* string_lit();
void skip_whitespaces();
void advance();
char curr_char();
char next();
void initlexer(char *);
//initialize JAM lexer
void initlexer(char *src){
    lexer.source = src;
    lexer.line = 1;
    lexer.pos = 0;
    lexer.col = 1;
}
//returns current character
char curr_char(){
    return lexer.source[lexer.pos];
}
// advance's pointer over tokens
void advance(){
    if(curr_char()=='\n'){
        lexer.line++;
        lexer.col = 1;
    }
    else lexer.col++;
    lexer.pos++;
}
//move's to the next character
char next(){
    return lexer.source[lexer.pos + 1] != '\0' ? lexer.source[lexer.pos + 1] : '\0';
}
//skip whitespace's
void skip_whitespaces(){
    while(isspace(curr_char())) advance();
}
//skip comments in the source code
void skip_comments(){
    if(curr_char()=='*' && next()=='*'){
        advance();
        advance();
        while(curr_char()!='\n' && next()!='\0'){
            advance();
        }
        return ;
    }
    //multiline comments
    if(curr_char()=='*'&& next()=='-'){
        advance();
        advance();
        while(!(curr_char() == '-' && next() == '*') && curr_char() != '\0'){
            advance();
        }
        if(curr_char() == '-' && next() == '*'){
            advance();
            advance();
        }
        return ;
    }
}
//create a Token
Token* create_token(TokenType type,char *lexeme,int line,int col){
    Token *token = (Token *)malloc(sizeof(Token));
    token->type = type;
    token->lexeme = strdup(lexeme);
    token->line=line;
    token->col=col;
    return token;   
}
Token* identifier(){
    int begin=lexer.pos;
    int col=lexer.col;
    while(isalnum(curr_char()) || curr_char()=='_') advance();
    int len = lexer.pos - begin;
    char *lexeme = (char*)malloc(len+1);
    strncpy(lexeme,lexer.source+begin,len);
    lexeme[len]='\0';
    TokenType type = TOKEN_IDENTIFIER;
    if (strcmp(lexeme, "fn") == 0)
        type = TOKEN_KEYWORD_FN;
    else if (strcmp(lexeme, "if") == 0)
        type = TOKEN_KEYWORD_IF;
    else if (strcmp(lexeme, "else") == 0)
        type = TOKEN_KEYWORD_ELSE;
    else if (strcmp(lexeme, "var") == 0)
        type = TOKEN_KEYWORD_VAR;
    else if (strcmp(lexeme, "return") == 0)
        type = TOKEN_KEYWORD_RETURN;
    else if (strcmp(lexeme, "import") == 0)
        type = TOKEN_KEYWORD_IMPORT;
    else if (strcmp(lexeme, "loop") == 0)
        type = TOKEN_KEYWORD_LOOP;
    else if (strcmp(lexeme, "forloop") == 0)
        type = TOKEN_KEYWORD_FORLOOP;
    else if (strcmp(lexeme, "Int") == 0)
        type = TOKEN_KEYWORD_INT;
    else if (strcmp(lexeme, "Float") == 0)
        type = TOKEN_KEYWORD_FLOAT;
    else if (strcmp(lexeme, "Bool") == 0)
        type = TOKEN_KEYWORD_BOOL;
    else if (strcmp(lexeme, "Void") == 0)
        type = TOKEN_KEYWORD_VOID;
    else if (strcmp(lexeme, "String") == 0)
        type = TOKEN_KEYWORD_STRING;
    else if (strcmp(lexeme, "struct") == 0)
        type = TOKEN_KEYWORD_STRUCT;
    else if (strcmp(lexeme, "true") == 0)
        type = TOKEN_KEYWORD_TRUE;
    else if (strcmp(lexeme, "false") == 0)
        type = TOKEN_KEYWORD_FALSE;
    else if (strcmp(lexeme, "null") == 0)
        type = TOKEN_KEYWORD_NULL;
    Token* token=create_token(type,lexeme,lexer.line,col);
    return token;
}
Token* number(){
    int s=lexer.pos;
    int col=lexer.col;
    while(isdigit(curr_char())) advance();
    if(curr_char()=='.'){
        advance();
        while(isdigit(curr_char())) advance();
    }
    int len=lexer.pos-s;
    char *lexeme=(char*)malloc(len+1);
    strncpy(lexeme,lexer.source+s,len);
    lexeme[len]='\0';
    Token* token=create_token(TOKEN_NUMBER,lexeme,lexer.line,col);
    return token;
}
Token* string_lit(){
    int startCol=lexer.col;
    advance();// skip opening quote
    int start=lexer.pos;
    while (curr_char()!='"'&&curr_char()!='\0') {
        // Handle escape sequences by skipping the next character.
        if (curr_char() == '\\') {
            advance(); // skip '\'
            advance(); // skip escaped character
        }        
        advance();
    }
    int len=lexer.pos-start;
    char* lexeme=malloc(len + 1);
    strncpy(lexeme,lexer.source+start,len);
    lexeme[len]='\0';
    if (curr_char()=='"') {
        advance(); // skip closing quote
    }
    Token* token = create_token(TOKEN_STRING, lexeme, lexer.line, startCol);
    return token;
}
Token* get_next_token()
{
    while (curr_char() != '\0') {
        // Skip whitespace.
        if (isspace(curr_char())) {
            skip_whitespaces();
            continue;
        }
        // Check and skip comments.
        if (curr_char() == '*' && (next() == '*' || next() == '-')) {
            skip_comments();
            continue;
        }
        int tokenLine = lexer.line;
        int tokenCol = lexer.col;
        char c = curr_char();

        // Recognize identifiers and keywords.
        if (isalpha(c) || c == '_')
            return identifier();

        // Recognize numbers.
        if (isdigit(c))
            return number();

        // Recognize strings.
        if (c == '"')
            return string_lit();

        // Recognize operators and delimiters.
        switch (c) {
            case '+': {
                advance();
                return create_token(TOKEN_OPERATOR_PLUS, "+", tokenLine, tokenCol);
            }
            case '-': {
                advance();
                // Check for arrow operator "->"
                if (curr_char() == '>') {
                    advance();
                    return create_token(TOKEN_ARROW, "->", tokenLine, tokenCol);
                }
                return create_token(TOKEN_OPERATOR_MINUS, "-", tokenLine, tokenCol);
            }
            case '*': {
                advance();
                return create_token(TOKEN_OPERATOR_MUL, "*", tokenLine, tokenCol);
            }
            case '/': {
                advance();
                return create_token(TOKEN_OPERATOR_DIV, "/", tokenLine, tokenCol);
            }
            case '%': {
                advance();
                return create_token(TOKEN_OPERATOR_MOD, "%", tokenLine, tokenCol);
            }
            case '=': {
                advance();
                if (curr_char() == '=') {
                    advance();
                    return create_token(TOKEN_OPERATOR_EQ, "==", tokenLine, tokenCol);
                }
                return create_token(TOKEN_OPERATOR_ASSIGN, "=", tokenLine, tokenCol);
            }
            case '!': {
                advance();
                if (curr_char() == '=') {
                    advance();
                    return create_token(TOKEN_OPERATOR_NEQ, "!=", tokenLine, tokenCol);
                }
                return create_token(TOKEN_OPERATOR_NOT, "!", tokenLine, tokenCol);
            }
            case '<': {
                advance();
                if (curr_char() == '=') {
                    advance();
                    return create_token(TOKEN_OPERATOR_LTE, "<=", tokenLine, tokenCol);
                }
                return create_token(TOKEN_OPERATOR_LT, "<", tokenLine, tokenCol);
            }
            case '>': {
                advance();
                if (curr_char() == '=') {
                    advance();
                    return create_token(TOKEN_OPERATOR_GTE, ">=", tokenLine, tokenCol);
                }
                return create_token(TOKEN_OPERATOR_GT, ">", tokenLine, tokenCol);
            }
            case '&': {
                advance();
                if (curr_char() == '&') {
                    advance();
                    return create_token(TOKEN_OPERATOR_AND, "&&", tokenLine, tokenCol);
                }
                break;
            }
            case '|': {
                advance();
                if (curr_char() == '|') {
                    advance();
                    return create_token(TOKEN_OPERATOR_OR, "||", tokenLine, tokenCol);
                }
                break;
            }
            case '(': {
                advance();
                return create_token(TOKEN_DELIM_OPEN_PAREN, "(", tokenLine, tokenCol);
            }
            case ')': {
                advance();
                return create_token(TOKEN_DELIM_CLOSE_PAREN, ")", tokenLine, tokenCol);
            }
            case '{': {
                advance();
                return create_token(TOKEN_DELIM_OPEN_BRACE, "{", tokenLine, tokenCol);
            }
            case '}': {
                advance();
                return create_token(TOKEN_DELIM_CLOSE_BRACE, "}", tokenLine, tokenCol);
            }
            case '[': {
                advance();
                return create_token(TOKEN_DELIM_OPEN_SQUARE, "[", tokenLine, tokenCol);
            }
            case ']': {
                advance();
                return create_token(TOKEN_DELIM_CLOSE_SQUARE, "]", tokenLine, tokenCol);
            }
            case ',': {
                advance();
                return create_token(TOKEN_DELIM_COMMA, ",", tokenLine, tokenCol);
            }
            case ':': {
                advance();
                return create_token(TOKEN_DELIM_COLON, ":", tokenLine, tokenCol);
            }
            case ';': {
                advance();
                return create_token(TOKEN_DELIM_SEMICOLON, ";", tokenLine, tokenCol);
            }
            case '.': {
                advance();
                return create_token(TOKEN_DELIM_DOT, ".", tokenLine, tokenCol);
            }
            default:
                // Handle unrecognized characters.
                fprintf(stderr, "Unrecognized character: '%c' at line %d, col %d\n", c, tokenLine, tokenCol);
                advance();
                break;
        }
    }
    return create_token(TOKEN_EOF, "EOF", lexer.line, lexer.col);
}
/*
int main() {
    char* source = "fn main() {\n"
                   "    var arr:[Int]=[1,2,3,4]\n"
                   "    var y = 20;\n"
                   "    var z = x + y;\n"
                   "    return arr[0];\n"
                   "}\n";
    initlexer(source);
    Token* token;
    while ((token = get_next_token())->type != TOKEN_EOF) {
        printf("<Token: %d, Lexeme: %s, Line: %d, Col: %d>\n",token->type, token->lexeme, token->line, token->col);
        free(token->lexeme);
        free(token);
    }
    free(token->lexeme);
    free(token);
    return 0;
}
*/