#pragma once

#include <luao.hpp>
#include <loapi.hpp>
#include <string>
#include <vector>
#include <stdexcept>

#ifndef LUAO_ENV
#define LUAO_ENV "_ENV"
#endif

#define FIRST_RESERVED 256

enum class Token 
{
    /* keywords */
    AND, BREAK, DO, EACH, ELSE, ELSEIF, END, FALSE, FOR, FUNCTION,
    GOTO, IF, IN, LOCAL, NIL, NOT, OR, REPEAT, RETURN, THEN, TRUE,
    UNTIL, WHILE, CLASS, OBJECT, TYPE, THROW, TRY, CATCH, FINALLY,
    /* symbols */
    PLUS, MINUS, MULTIPLY, DIVIDE, IDIV, MODULO, SHL, SHR,
    EQ, NE, LT, LE, GT, GE, ASSIGN, LPAREN, RPAREN,
    LBRACE, RBRACE, LBRACKET, RBRACKET, SEMICOLON, COLON,
    COLON_DB, COMMA, DOT, DOT_DB, SHARP, CARET, VARARG, NULLABLE,
    ARROW,
    /* others */
    INT, FLOAT, STRING, IDENTIFIER, EOS
};

constexpr const char* TokenNames[] = {
    "AND","BREAK","DO","EACH","ELSE","ELSEIF","END","FALSE","FOR","FUNCTION",
    "GOTO","IF","IN","LOCAL","NIL","NOT","OR","REPEAT","RETURN","THEN","TRUE",
    "UNTIL","WHILE","CLASS","OBJECT","TYPE","THROW","TRY","CATCH","FINALLY",
    "PLUS","MINUS","MULTIPLY","DIVIDE","IDIV","MODULO","SHL","SHR",
    "EQ","NE","LT","LE","GT","GE","ASSIGN","LPAREN","RPAREN",
    "LBRACE","RBRACE","LBRACKET","RBRACKET","SEMICOLON","COLON",
    "COLON_DB","COMMA","DOT","DOT_DB","SHARP","CARET","VARARG", "NULLABLE",
    "ARROW",
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