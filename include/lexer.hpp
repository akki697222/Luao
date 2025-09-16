#pragma once

#include <luao.hpp>
#include <string>
#include <vector>
#include <stdexcept>

#ifndef LUAO_ENV
#define LUAO_ENV "_ENV"
#endif

enum class Token 
{
    /* keywords */
    AND, BREAK, DO, ELSE, ELSEIF, END, FALSE, FOR, FUNCTION,
    GOTO, IF, IN, LOCAL, NIL, NOT, OR, REPEAT, RETURN, THEN, TRUE,
    UNTIL, WHILE,
    /* symbols */
    PLUS, MINUS, MULTIPLY, DIVIDE, IDIV, MODULO, POW, LEN,
    BAND, BOR, BXOR, BNOT, SHL, SHR,
    EQ, NE, LT, LE, GT, GE, ASSIGN, LPAREN, RPAREN,
    LBRACE, RBRACE, LBRACKET, RBRACKET, SEMICOLON, COLON,
    COLON_DB, COMMA, DOT, CONCAT, VARARG,
    /* others */
    INT, FLOAT, STRING, IDENTIFIER, EOS
};

constexpr const char* TokenNames[] = {
    "AND","BREAK","DO","ELSE","ELSEIF","END","FALSE","FOR","FUNCTION",
    "GOTO","IF","IN","LOCAL","NIL","NOT","OR","REPEAT","RETURN","THEN","TRUE",
    "UNTIL","WHILE",
    "PLUS","MINUS","MULTIPLY","DIVIDE","IDIV","MODULO","POW","LEN",
    "BAND","BOR","BXOR","BNOT","SHL","SHR",
    "EQ","NE","LT","LE","GT","GE","ASSIGN","LPAREN","RPAREN",
    "LBRACE","RBRACE","LBRACKET","RBRACKET","SEMICOLON","COLON",
    "COLON_DB","COMMA","DOT","CONCAT","VARARG",
    "INT","FLOAT","STRING","IDENTIFIER","EOS"
};

struct TokenInfo {
    Token type;
    std::string value;
    int line;
};

class Lexer {
public:
    Lexer(const std::string& source);
    
    TokenInfo nextToken();
    TokenInfo peek();

private:
    std::string source_;
    size_t pos_;
    int line_;

    void advance();
    char peekChar() const;
    void skipWhitespace();
    void skipComment();
    void skipLongComment();
    TokenInfo readIdentifierOrKeyword();
    TokenInfo readNumber();
    TokenInfo readString();
    void throwError(const std::string& msg);
    bool isAlpha(char c) const;
    bool isDigit(char c) const;
    bool isHexDigit(char c) const;
};