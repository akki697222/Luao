#pragma once

#include <luao.hpp>
#include <lexer.hpp>
#include <vector>
#include <string>
#include <memory>
#include <map>

class AstNode
{
public:
    virtual ~AstNode() = default;
    virtual std::string dump(int indent = 0) const = 0;
};

class Expression : public AstNode
{
public:
    virtual ~Expression() = default;
};

class Statement : public AstNode
{
public:
    virtual ~Statement() = default;
};

class Block : public AstNode
{
public:
    std::vector<std::unique_ptr<Statement>> statements_;
    void addStatement(std::unique_ptr<Statement> stmt);
    Block() = default;
    std::string dump(int indent) const override;
};

class BinaryExpr : public Expression
{
public:
    std::unique_ptr<Expression> left_;
    TokenInfo op_;
    std::unique_ptr<Expression> right_;
    BinaryExpr(std::unique_ptr<Expression> left, TokenInfo op, std::unique_ptr<Expression> right);
    std::string dump(int indent) const override;
};

class UnaryExpr : public Expression
{
public:
    TokenInfo op_;
    std::unique_ptr<Expression> operand_;
    UnaryExpr(TokenInfo op, std::unique_ptr<Expression> operand);
    std::string dump(int indent) const override;
};

class NumberLiteral : public Expression
{
public:
    double value_;
    NumberLiteral(double value);
    std::string dump(int indent) const override;
};

class StringLiteral : public Expression
{
public:
    std::string value_;
    StringLiteral(std::string value);
    std::string dump(int indent) const override;
};

class Identifier : public Expression
{
public:
    std::string name_;
    Identifier(std::string name);
    std::string dump(int indent) const override;
};

class AssignStatement : public Statement
{
public:
    std::vector<std::unique_ptr<Expression>> targets_;
    std::vector<std::unique_ptr<Expression>> values_;
    AssignStatement(std::vector<std::unique_ptr<Expression>> targets,
                    std::vector<std::unique_ptr<Expression>> values);
    std::string dump(int indent) const override;
};

class IfStatement : public Statement
{
public:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Block> then_body_;
    std::unique_ptr<Block> else_body_;
    IfStatement(std::unique_ptr<Expression> condition,
                std::unique_ptr<Block> then_body,
                std::unique_ptr<Block> else_body = nullptr);
    std::string dump(int indent) const override;
};

class WhileStatement : public Statement
{
public:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Block> body_;
    WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Block> body);
    std::string dump(int indent) const override;
};

class BreakStatement : public Statement
{
public:
    std::string dump(int indent) const override;
};

class FunctionStatement : public Statement
{   
public:
    std::string name_;
    std::vector<std::string> params_;
    std::vector<std::string> param_types_;
    std::vector<std::string> return_types_;
    std::string attribute_;
    std::unique_ptr<Block> body_;
    FunctionStatement(std::string name, std::vector<std::string> params, std::vector<std::string> param_types, std::vector<std::string> return_types, std::string attribute, std::unique_ptr<Block> body);
    std::string dump(int indent) const override;
};

class LocalStatement : public Statement
{
public:
    std::vector<std::string> names_;
    std::vector<std::unique_ptr<Expression>> values_;
    std::vector<std::string> attributes_;
    std::vector<std::string> types_;
    LocalStatement(std::vector<std::string> names, std::vector<std::unique_ptr<Expression>> values);
    std::string dump(int indent) const override;
};

class Parser
{
public:
    Parser(std::vector<TokenInfo> tokens);
    std::unique_ptr<Block> parse();

private:
    std::vector<TokenInfo> tokens_;
    size_t pos_;

    const TokenInfo &peek() const;
    TokenInfo consume(Token type, const std::string &value = "");
    void expect(Token type, const std::string &value = "");
    bool match(Token type, const std::string &value = "") const;
    void error(const std::string &message) const;

    std::unique_ptr<Block> parseProgram();
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<Block> parseBlock();
    std::unique_ptr<AssignStatement> parseAssignStatement();
    std::unique_ptr<IfStatement> parseIfStatement();
    std::unique_ptr<WhileStatement> parseWhileStatement();
    std::unique_ptr<BreakStatement> parseBreakStatement();
    std::unique_ptr<Expression> parseExpression(int min_precedence = 0);
    std::unique_ptr<Expression> parsePrimary();
};

struct OpPriority {
    int left;
    int right;
};

static const std::map<Token, OpPriority> priority = {
    {Token::PLUS, {10, 10}},
    {Token::MINUS, {10, 10}},
    {Token::MULTIPLY, {11, 11}},
    {Token::DIVIDE, {11, 11}},
    {Token::IDIV, {11, 11}},
    {Token::MODULO, {11, 11}},
    {Token::CARET, {14, 13}},
    {Token::EQ, {3, 3}},
    {Token::NE, {3, 3}},
    {Token::LT, {3, 3}},
    {Token::LE, {3, 3}},
    {Token::GT, {3, 3}},
    {Token::GE, {3, 3}},
    {Token::AND, {2, 2}},
    {Token::OR, {1, 1}},
    {Token::CONCAT, {10, 10}}
};