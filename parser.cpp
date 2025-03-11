#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <memory>

// --- Token Definitions ---

enum class TokenType {
    // Keywords
    FN, IF, ELSE, VAR, RETURN, IMPORT, LOOP, FORLOOP,
    // Literals and identifiers
    IDENTIFIER, INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL,
    // Operators
    OP_PLUS, OP_MINUS, OP_MULT, OP_DIV, OP_MOD,
    OP_ASSIGN, OP_EQ, OP_NEQ, OP_LT, OP_LTE, OP_GT, OP_GTE,
    OP_AND, OP_OR, OP_NOT,
    // Special tokens
    ARROW,       // ->
    LPAREN, RPAREN,
    LBRACE, RBRACE,
    LBRACKET, RBRACKET,
    COMMA, COLON, SEMICOLON, PERIOD,
    // Data type keywords (could also be IDENTIFIERs)
    KEYWORD_INT, KEYWORD_FLOAT, KEYWORD_BOOL, KEYWORD_STRING, KEYWORD_VOID,
    STRUCT,
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
};

// --- AST Definitions ---

enum class ASTNodeType {
    Program,
    FunctionDecl,
    VarDecl,
    IfStmt,
    ReturnStmt,
    BinaryExpr,
    Literal,
    Identifier
};

struct ASTNode {
    ASTNodeType nodeType;
    std::vector<std::unique_ptr<ASTNode>> children;
    std::string value; // e.g. function name, identifier, operator symbol, literal value

    ASTNode(ASTNodeType type) : nodeType(type) {}
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

// --- Parser Implementation ---

class Parser {
public:
    Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

    ASTNodePtr parseProgram();

private:
    const std::vector<Token>& tokens;
    size_t pos;

    const Token& currentToken() const {
        if (pos < tokens.size())
            return tokens[pos];
        throw std::runtime_error("Unexpected end of token stream");
    }

    bool match(TokenType expected) {
        if (currentToken().type == expected) {
            pos++;
            return true;
        }
        return false;
    }

    void expect(TokenType expected, const std::string& errMsg) {
        if (!match(expected)) {
            throw std::runtime_error("Parse error: " + errMsg +
                                     " at line " + std::to_string(currentToken().line));
        }
    }

    // Parsing functions:
    ASTNodePtr parseFunctionDeclaration();
    ASTNodePtr parseParameterList();
    ASTNodePtr parseReturnType();
    ASTNodePtr parseBlock();
    ASTNodePtr parseStatement();
    ASTNodePtr parseVarDeclaration();
    ASTNodePtr parseIfStatement();
    ASTNodePtr parseReturnStatement();
    ASTNodePtr parseExpression();
    ASTNodePtr parsePrimary();
    ASTNodePtr parseBinaryOpRHS(int exprPrec, ASTNodePtr lhs);
    int getPrecedence(TokenType type);
};

int Parser::getPrecedence(TokenType type) {
    // Basic operator precedence: higher number means higher precedence
    switch(type) {
        case TokenType::OP_MULT:
        case TokenType::OP_DIV:
        case TokenType::OP_MOD:
            return 2;
        case TokenType::OP_PLUS:
        case TokenType::OP_MINUS:
            return 1;
        default:
            return 0;
    }
}

ASTNodePtr Parser::parseProgram() {
    auto root = std::make_unique<ASTNode>(ASTNodeType::Program);
    // Parse until we hit the end-of-file token.
    while (currentToken().type != TokenType::END_OF_FILE) {
        if (currentToken().type == TokenType::FN) {
            root->children.push_back(parseFunctionDeclaration());
        } else if (currentToken().type == TokenType::VAR) {
            root->children.push_back(parseVarDeclaration());
        } else {
            // For simplicity, treat any other token as the start of an expression statement.
            auto stmt = parseStatement();
            root->children.push_back(std::move(stmt));
        }
    }
    return root;
}

ASTNodePtr Parser::parseFunctionDeclaration() {
    // Expected grammar: 
    // fn <identifier> ( <parameter_list> ) -> <ReturnType> { <block> }
    expect(TokenType::FN, "Expected 'fn' keyword");
    auto funcNode = std::make_unique<ASTNode>(ASTNodeType::FunctionDecl);

    // Function name
    if (currentToken().type != TokenType::IDENTIFIER)
        throw std::runtime_error("Expected function name after 'fn'");
    funcNode->value = currentToken().lexeme;
    pos++;

    expect(TokenType::LPAREN, "Expected '(' after function name");

    // Parameter list
    auto params = parseParameterList();
    if (params) {
        funcNode->children.push_back(std::move(params));
    }
    expect(TokenType::RPAREN, "Expected ')' after parameter list");

    expect(TokenType::ARROW, "Expected '->' after parameter list");

    // Return type
    auto retType = parseReturnType();
    if (retType) {
        funcNode->children.push_back(std::move(retType));
    }

    expect(TokenType::LBRACE, "Expected '{' to start function body");
    auto body = parseBlock();
    funcNode->children.push_back(std::move(body));
    expect(TokenType::RBRACE, "Expected '}' to end function body");

    return funcNode;
}

ASTNodePtr Parser::parseParameterList() {
    // Parse a comma-separated list: (<identifier> : <type> (, <identifier> : <type>)*)?
    auto paramsNode = std::make_unique<ASTNode>(ASTNodeType::Program); // Container for parameters
    while (currentToken().type == TokenType::IDENTIFIER) {
        auto param = std::make_unique<ASTNode>(ASTNodeType::VarDecl);
        // Parameter name
        param->value = currentToken().lexeme;
        pos++;
        expect(TokenType::COLON, "Expected ':' after parameter name");
        if (currentToken().type != TokenType::IDENTIFIER)
            throw std::runtime_error("Expected type identifier in parameter list");
        auto typeNode = std::make_unique<ASTNode>(ASTNodeType::Identifier);
        typeNode->value = currentToken().lexeme;
        pos++;
        param->children.push_back(std::move(typeNode));
        paramsNode->children.push_back(std::move(param));
        if (!match(TokenType::COMMA))
            break;
    }
    return paramsNode;
}

ASTNodePtr Parser::parseReturnType() {
    // For simplicity, a return type is an identifier
    if (currentToken().type != TokenType::IDENTIFIER)
        throw std::runtime_error("Expected return type identifier");
    auto retType = std::make_unique<ASTNode>(ASTNodeType::Identifier);
    retType->value = currentToken().lexeme;
    pos++;
    return retType;
}

ASTNodePtr Parser::parseBlock() {
    // A block: a sequence of statements until the closing '}'
    auto blockNode = std::make_unique<ASTNode>(ASTNodeType::Program);
    while (currentToken().type != TokenType::RBRACE &&
           currentToken().type != TokenType::END_OF_FILE) {
        blockNode->children.push_back(parseStatement());
    }
    return blockNode;
}

ASTNodePtr Parser::parseStatement() {
    // A statement can be a variable declaration, if statement, return statement, or expression statement.
    if (currentToken().type == TokenType::VAR) {
        return parseVarDeclaration();
    } else if (currentToken().type == TokenType::IF) {
        return parseIfStatement();
    } else if (currentToken().type == TokenType::RETURN) {
        return parseReturnStatement();
    } else {
        // For expression statements, we parse an expression and expect a semicolon.
        auto expr = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after expression");
        return expr;
    }
}

ASTNodePtr Parser::parseVarDeclaration() {
    // Grammar: var <identifier> : <type> = <expression> ;
    expect(TokenType::VAR, "Expected 'var' keyword");
    auto varDecl = std::make_unique<ASTNode>(ASTNodeType::VarDecl);
    if (currentToken().type != TokenType::IDENTIFIER)
        throw std::runtime_error("Expected variable name");
    varDecl->value = currentToken().lexeme;
    pos++;
    expect(TokenType::COLON, "Expected ':' after variable name");
    if (currentToken().type != TokenType::IDENTIFIER)
        throw std::runtime_error("Expected type in variable declaration");
    auto typeNode = std::make_unique<ASTNode>(ASTNodeType::Identifier);
    typeNode->value = currentToken().lexeme;
    pos++;
    varDecl->children.push_back(std::move(typeNode));
    expect(TokenType::OP_ASSIGN, "Expected '=' in variable declaration");
    auto expr = parseExpression();
    varDecl->children.push_back(std::move(expr));
    expect(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    return varDecl;
}

ASTNodePtr Parser::parseIfStatement() {
    // Grammar: if <expression> { <block> } (else { <block> })?
    expect(TokenType::IF, "Expected 'if'");
    auto ifNode = std::make_unique<ASTNode>(ASTNodeType::IfStmt);
    auto condition = parseExpression();
    ifNode->children.push_back(std::move(condition));
    expect(TokenType::LBRACE, "Expected '{' after if condition");
    auto thenBlock = parseBlock();
    ifNode->children.push_back(std::move(thenBlock));
    expect(TokenType::RBRACE, "Expected '}' after if block");
    if (currentToken().type == TokenType::ELSE) {
        match(TokenType::ELSE);
        expect(TokenType::LBRACE, "Expected '{' after else");
        auto elseBlock = parseBlock();
        ifNode->children.push_back(std::move(elseBlock));
        expect(TokenType::RBRACE, "Expected '}' after else block");
    }
    return ifNode;
}

ASTNodePtr Parser::parseReturnStatement() {
    // Grammar: return <expression> ;
    expect(TokenType::RETURN, "Expected 'return'");
    auto returnNode = std::make_unique<ASTNode>(ASTNodeType::ReturnStmt);
    auto expr = parseExpression();
    returnNode->children.push_back(std::move(expr));
    expect(TokenType::SEMICOLON, "Expected ';' after return statement");
    return returnNode;
}

// --- Expression Parsing ---
// For simplicity, we use a basic operator-precedence parser.

ASTNodePtr Parser::parseExpression() {
    auto lhs = parsePrimary();
    return parseBinaryOpRHS(0, std::move(lhs));
}

ASTNodePtr Parser::parseBinaryOpRHS(int exprPrec, ASTNodePtr lhs) {
    while (true) {
        int tokPrec = getPrecedence(currentToken().type);
        if (tokPrec < exprPrec)
            return lhs;
        // It's an operator token.
        Token opToken = currentToken();
        pos++; // consume operator
        auto rhs = parsePrimary();
        int nextPrec = getPrecedence(currentToken().type);
        if (tokPrec < nextPrec)
            rhs = parseBinaryOpRHS(tokPrec + 1, std::move(rhs));
        auto binaryNode = std::make_unique<ASTNode>(ASTNodeType::BinaryExpr);
        binaryNode->value = opToken.lexeme;
        binaryNode->children.push_back(std::move(lhs));
        binaryNode->children.push_back(std::move(rhs));
        lhs = std::move(binaryNode);
    }
    return lhs;
}

ASTNodePtr Parser::parsePrimary() {
    // Primary: literal, identifier, or parenthesized expression.
    Token token = currentToken();
    if (token.type == TokenType::INT_LITERAL || token.type == TokenType::FLOAT_LITERAL ||
        token.type == TokenType::STRING_LITERAL) {
        pos++;
        auto literalNode = std::make_unique<ASTNode>(ASTNodeType::Literal);
        literalNode->value = token.lexeme;
        return literalNode;
    } else if (token.type == TokenType::IDENTIFIER) {
        pos++;
        auto idNode = std::make_unique<ASTNode>(ASTNodeType::Identifier);
        idNode->value = token.lexeme;
        return idNode;
    } else if (token.type == TokenType::LPAREN) {
        pos++; // consume '('
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    } else {
        throw std::runtime_error("Unexpected token in expression: " + token.lexeme);
    }
}

// --- Main Function for Testing ---

int main() {
    // Example token stream for:
    // fn factorial(n: Int) -> Int {
    //     if n <= 1 {
    //         return 1;
    //     } else {
    //         return n * factorial(n - 1);
    //     }
    // }
    // var result:Int = factorial(5);
    std::vector<Token> tokens = {
        {TokenType::FN, "fn", 1, 1}
    };

    try {
        Parser parser(tokens);
        ASTNodePtr ast = parser.parseProgram();
        std::cout << "Parsing successful!" << std::endl;
        // Traverse and print the AST as needed.
    } catch (const std::exception& e) {
        std::cerr << "Parser error: " << e.what() << std::endl;
    }
    return 0;
}
