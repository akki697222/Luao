#pragma once

#include "lexer.hpp"
#include <memory>
#include <string>
#include <vector>
#include <map>

// --- Base Classes ---
class AstNode {
public:
    virtual ~AstNode() = default;
};
class Expression : public AstNode {};
class Statement : public AstNode {};

class Block : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements_;
};

// --- Literals ---
class NilLiteral : public Expression {};
class BoolLiteral : public Expression {
public:
    bool value_;
    explicit BoolLiteral(bool value) : value_(value) {}
};
class NumberLiteral : public Expression {
public:
    std::string value_; // Keep as string to preserve hex/dec format
    explicit NumberLiteral(const std::string& value) : value_(value) {}
};
class StringLiteral : public Expression {
public:
    std::string value_;
    explicit StringLiteral(const std::string& value) : value_(value) {}
};
class VarargLiteral : public Expression {};

// --- Expressions ---
class Identifier : public Expression {
public:
    std::string name_;
    std::string attribute_;
    explicit Identifier(const std::string& name, std::string attribute = "") : name_(name), attribute_(std::move(attribute)) {}
};

class FunctionDef : public Expression {
public:
    std::vector<std::unique_ptr<Identifier>> params_;
    bool is_vararg_;
    std::unique_ptr<Block> body_;
};

class BinaryExpr : public Expression {
public:
    std::unique_ptr<Expression> left_;
    TokenInfo op_;
    std::unique_ptr<Expression> right_;
    BinaryExpr(std::unique_ptr<Expression> left, TokenInfo op, std::unique_ptr<Expression> right)
        : left_(std::move(left)), op_(op), right_(std::move(right)) {}
};

class UnaryExpr : public Expression {
public:
    TokenInfo op_;
    std::unique_ptr<Expression> operand_;
    UnaryExpr(TokenInfo op, std::unique_ptr<Expression> operand)
        : op_(op), operand_(std::move(operand)) {}
};

class FunctionCall : public Expression {
public:
    std::unique_ptr<Expression> prefix_expr_;
    std::vector<std::unique_ptr<Expression>> args_;
    std::unique_ptr<StringLiteral> method_name_; // for method calls like `a:b()`
    FunctionCall(std::unique_ptr<Expression> prefix, std::vector<std::unique_ptr<Expression>> args)
        : prefix_expr_(std::move(prefix)), args_(std::move(args)), method_name_(nullptr) {}
};

class TableAccess : public Expression {
public:
    std::unique_ptr<Expression> prefix_expr_;
    std::unique_ptr<Identifier> field_name_;
    TableAccess(std::unique_ptr<Expression> prefix, std::unique_ptr<Identifier> field)
        : prefix_expr_(std::move(prefix)), field_name_(std::move(field)) {}
};

class IndexAccess : public Expression {
public:
    std::unique_ptr<Expression> prefix_expr_;
    std::unique_ptr<Expression> index_expr_;
    IndexAccess(std::unique_ptr<Expression> prefix, std::unique_ptr<Expression> index)
        : prefix_expr_(std::move(prefix)), index_expr_(std::move(index)) {}
};

struct TableField {
    std::unique_ptr<Expression> key; // Can be nullptr for list part
    std::unique_ptr<Expression> value;
};

class TableConstructor : public Expression {
public:
    std::vector<TableField> fields_;
};

// --- Statements ---
class AssignStatement : public Statement {
public:
    std::vector<std::unique_ptr<Expression>> targets_;
    std::vector<std::unique_ptr<Expression>> values_;
};

class LocalStatement : public Statement {
public:
    std::vector<std::unique_ptr<Identifier>> names_;
    std::vector<std::unique_ptr<Expression>> values_;
};

class ExprStatement : public Statement {
public:
    std::unique_ptr<Expression> expr_;
    explicit ExprStatement(std::unique_ptr<Expression> expr) : expr_(std::move(expr)) {}
};

struct IfClause {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Block> body;
};

class IfStatement : public Statement {
public:
    std::vector<IfClause> if_clauses_;
    std::unique_ptr<Block> else_body_; // Can be nullptr
};

class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Block> body_;
};

class DoStatement : public Statement {
public:
    std::unique_ptr<Block> body_;
};

class RepeatUntilStatement : public Statement {
public:
    std::unique_ptr<Block> body_;
    std::unique_ptr<Expression> condition_;
};

class BreakStatement : public Statement {};

class ReturnStatement : public Statement {
public:
    std::vector<std::unique_ptr<Expression>> exprs_;
};

class GotoStatement : public Statement {
public:
    std::string name_;
    explicit GotoStatement(std::string name) : name_(std::move(name)) {}
};

class LabelStatement : public Statement {
public:
    std::string name_;
    explicit LabelStatement(std::string name) : name_(std::move(name)) {}
};

class NumericForStatement : public Statement {
public:
    std::unique_ptr<Identifier> var_;
    std::unique_ptr<Expression> start_;
    std::unique_ptr<Expression> end_;
    std::unique_ptr<Expression> step_; // Can be nullptr
    std::unique_ptr<Block> body_;
};

class GenericForStatement : public Statement {
public:
    std::vector<std::unique_ptr<Identifier>> names_;
    std::vector<std::unique_ptr<Expression>> exprs_;
    std::unique_ptr<Block> body_;
};

class FunctionStatement : public Statement {
public:
    std::unique_ptr<Expression> name_;
    std::unique_ptr<FunctionDef> def_;
    bool is_local_;
};

// --- Parser ---
class Parser {
public:
    explicit Parser(const std::string& source);
    std::unique_ptr<Block> parse();

private:
    Lexer lexer_;
    TokenInfo current_token_;
    TokenInfo lookahead_token_;

    void advance();
    void consume(Token type);
    bool match(Token type);
    bool check(Token type);
    void error(const std::string& message);

    // Statement Parsers
    std::unique_ptr<Block> parseBlock();
    std::unique_ptr<Statement> parseStatement();
    std::vector<std::unique_ptr<Statement>> parseStatementList();
    std::unique_ptr<Statement> parseLocalStatement();
    std::unique_ptr<Statement> parseFunctionStatement();
    std::unique_ptr<Statement> parseIfStatement();
    std::unique_ptr<Statement> parseWhileStatement();
    std::unique_ptr<Statement> parseDoStatement();
    std::unique_ptr<Statement> parseForStatement();
    std::unique_ptr<Statement> parseRepeatStatement();
    std::unique_ptr<Statement> parseReturnStatement();
    std::unique_ptr<Statement> parseBreakStatement();
    std::unique_ptr<Statement> parseGotoStatement();
    std::unique_ptr<Statement> parseLabelStatement();
    std::unique_ptr<Statement> parseAssignOrCallStatement();

    // Expression Parsers
    std::unique_ptr<Expression> parseExpression(int precedence = 0);
    std::unique_ptr<Expression> parsePrefixExpression();
    std::unique_ptr<Expression> parseSimpleExpression();
    std::unique_ptr<Expression> parseSuffixedExpression(std::unique_ptr<Expression> prefix);
    std::unique_ptr<FunctionDef> parseFunctionDef();
    void parseFunctionArgs(std::vector<std::unique_ptr<Expression>>& args);
    std::unique_ptr<TableConstructor> parseTableConstructor();
    std::unique_ptr<Identifier> parseIdentifier(bool can_have_attr = false);
    std::string parseAttribute();
};