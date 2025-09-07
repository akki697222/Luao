#include "parser.hpp"
#include <sstream>

void Block::addStatement(std::unique_ptr<Statement> stmt) {
    statements_.push_back(std::move(stmt));
}

std::string Block::dump(int indent) const {
    std::string s = std::string(indent, ' ') + "Block(\n";
    for (const auto &stmt : statements_)
        s += stmt->dump(indent + 2) + ",\n";
    s += std::string(indent, ' ') + ")";
    return s;
}

BinaryExpr::BinaryExpr(std::unique_ptr<Expression> left, TokenInfo op, std::unique_ptr<Expression> right)
    : left_(std::move(left)), op_(op), right_(std::move(right)) {}

std::string BinaryExpr::dump(int indent) const {
    std::string s = std::string(indent, ' ') + "BinaryExpr(\n";
    s += left_->dump(indent + 2) + ",\n";
    s += std::string(indent + 2, ' ') + op_.value + ",\n";
    s += right_->dump(indent + 2) + "\n";
    s += std::string(indent, ' ') + ")";
    return s;
}

UnaryExpr::UnaryExpr(TokenInfo op, std::unique_ptr<Expression> operand)
    : op_(op), operand_(std::move(operand)) {}

std::string UnaryExpr::dump(int indent) const {
    std::string s = std::string(indent, ' ') + "UnaryExpr(\n";
    s += std::string(indent + 2, ' ') + op_.value + ",\n";
    s += operand_->dump(indent + 2) + "\n";
    s += std::string(indent, ' ') + ")";
    return s;
}

NumberLiteral::NumberLiteral(double value) : value_(value) {}

std::string NumberLiteral::dump(int indent) const {
    return std::string(indent, ' ') + "NumberLiteral(" + std::to_string(value_) + ")";
}

StringLiteral::StringLiteral(std::string value) : value_(std::move(value)) {}

std::string StringLiteral::dump(int indent) const {
    return std::string(indent, ' ') + "StringLiteral(\"" + value_ + "\")";
}

Identifier::Identifier(std::string name) : name_(std::move(name)) {}

std::string Identifier::dump(int indent) const {
    return std::string(indent, ' ') + "Identifier(" + name_ + ")";
}

AssignStatement::AssignStatement(std::vector<std::unique_ptr<Expression>> targets,
                                std::vector<std::unique_ptr<Expression>> values)
    : targets_(std::move(targets)), values_(std::move(values)) {}

std::string AssignStatement::dump(int indent) const {
    std::string s = std::string(indent, ' ') + "AssignStatement(\n";
    s += std::string(indent + 2, ' ') + "targets: [\n";
    for (const auto &t : targets_)
        s += t->dump(indent + 4) + ",\n";
    s += std::string(indent + 2, ' ') + "],\n";
    s += std::string(indent + 2, ' ') + "values: [\n";
    for (const auto &v : values_)
        s += v->dump(indent + 4) + ",\n";
    s += std::string(indent + 2, ' ') + "]\n";
    s += std::string(indent, ' ') + ")";
    return s;
}

IfStatement::IfStatement(std::unique_ptr<Expression> condition,
                         std::unique_ptr<Block> then_body,
                         std::unique_ptr<Block> else_body)
    : condition_(std::move(condition)), then_body_(std::move(then_body)),
      else_body_(std::move(else_body)) {}

std::string IfStatement::dump(int indent) const {
    std::string s = std::string(indent, ' ') + "IfStatement(\n";
    s += condition_->dump(indent + 2) + ",\n";
    s += then_body_->dump(indent + 2) + ",\n";
    if (else_body_)
        s += else_body_->dump(indent + 2) + ",\n";
    s += std::string(indent, ' ') + ")";
    return s;
}

WhileStatement::WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Block> body)
    : condition_(std::move(condition)), body_(std::move(body)) {}

std::string WhileStatement::dump(int indent) const {
    std::string s = std::string(indent, ' ') + "WhileStatement(\n";
    s += condition_->dump(indent + 2) + ",\n";
    s += body_->dump(indent + 2) + "\n";
    s += std::string(indent, ' ') + ")";
    return s;
}

std::string BreakStatement::dump(int indent) const {
    return std::string(indent, ' ') + "BreakStatement()";
}

Parser::Parser(std::vector<TokenInfo> tokens) : tokens_(std::move(tokens)), pos_(0) {}

const TokenInfo& Parser::peek() const {
    if (pos_ >= tokens_.size()) {
        throw std::runtime_error("Unexpected end of input at line " +
                                 std::to_string(tokens_.back().line));
    }
    return tokens_[pos_];
}

TokenInfo Parser::consume(Token type, const std::string& value) {
    if (pos_ >= tokens_.size()) {
        error("Expected " + std::string(TokenNames[static_cast<int>(type)]) +
              (value.empty() ? "" : " '" + value + "'") + ", got EOS");
    }
    const TokenInfo& tok = tokens_[pos_];
    if (tok.type != type || (!value.empty() && tok.value != value)) {
        error("Expected " + std::string(TokenNames[static_cast<int>(type)]) +
              (value.empty() ? "" : " '" + value + "'") + ", got " +
              tok.value);
    }
    pos_++;
    return tok;
}

void Parser::expect(Token type, const std::string& value) {
    consume(type, value);
}

bool Parser::match(Token type, const std::string& value) const {
    if (pos_ >= tokens_.size()) return false;
    return tokens_[pos_].type == type && (value.empty() || tokens_[pos_].value == value);
}

void Parser::error(const std::string& message) const {
    const TokenInfo& tok = pos_ < tokens_.size() ? tokens_[pos_] : tokens_.back();
    throw std::runtime_error("luaoc: stdin:" + std::to_string(tok.line) + ": " + message);
}

std::unique_ptr<Block> Parser::parse() {
    return parseProgram();
}

std::unique_ptr<Block> Parser::parseProgram() {
    auto block = std::make_unique<Block>();
    while (!match(Token::EOS)) {
        if (match(Token::SEMICOLON)) {
            consume(Token::SEMICOLON);
            continue;
        }
        block->addStatement(parseStatement());
    }
    return block;
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (match(Token::IF)) {
        return parseIfStatement();
    } else if (match(Token::WHILE)) {
        return parseWhileStatement();
    } else if (match(Token::IDENTIFIER)) {
        return parseAssignStatement();
    } else if (match(Token::BREAK)) {
        return parseBreakStatement();
    }
    error("Expected statement");
    return nullptr;
}

std::unique_ptr<Block> Parser::parseBlock() {
    auto block = std::make_unique<Block>();
    while (!match(Token::END) && !match(Token::ELSE) && !match(Token::ELSEIF) &&
           !match(Token::UNTIL) && !match(Token::EOS)) {
        if (match(Token::SEMICOLON)) {
            consume(Token::SEMICOLON);
            continue;
        }
        block->addStatement(parseStatement());
    }
    return block;
}

std::unique_ptr<AssignStatement> Parser::parseAssignStatement() {
    std::vector<std::unique_ptr<Expression>> targets;
    std::vector<std::unique_ptr<Expression>> values;

    do {
        TokenInfo id = consume(Token::IDENTIFIER);
        targets.push_back(std::make_unique<Identifier>(id.value));
        if (!match(Token::COMMA)) break;
        consume(Token::COMMA);
    } while (match(Token::IDENTIFIER));

    if (!match(Token::ASSIGN)) {
        error("Expected '=' after variable list");
    }
    consume(Token::ASSIGN);

    do {
        values.push_back(parseExpression());
        if (!match(Token::COMMA)) break;
        consume(Token::COMMA);
    } while (!match(Token::SEMICOLON) && !match(Token::END) && !match(Token::EOS));

    return std::make_unique<AssignStatement>(std::move(targets), std::move(values));
}

std::unique_ptr<IfStatement> Parser::parseIfStatement() {
    consume(Token::IF);
    auto condition = parseExpression();
    expect(Token::THEN);
    auto then_body = parseBlock();
    std::unique_ptr<Block> else_body = nullptr;
    if (match(Token::ELSE)) {
        consume(Token::ELSE);
        else_body = parseBlock();
    }
    expect(Token::END);
    return std::make_unique<IfStatement>(std::move(condition), std::move(then_body), std::move(else_body));
}

std::unique_ptr<WhileStatement> Parser::parseWhileStatement() {
    consume(Token::WHILE);
    auto condition = parseExpression();
    expect(Token::DO);
    auto body = parseBlock();
    expect(Token::END);
    return std::make_unique<WhileStatement>(std::move(condition), std::move(body));
}

std::unique_ptr<BreakStatement> Parser::parseBreakStatement() {
    consume(Token::BREAK);
    return std::make_unique<BreakStatement>();
}

std::unique_ptr<Expression> Parser::parseExpression(int min_precedence) {
    auto expr = parsePrimary();
    while (pos_ < tokens_.size()) {
        const TokenInfo& tok = peek();
        auto it = priority.find(tok.type);
        if (it == priority.end() || it->second.left <= min_precedence) {
            break;
        }
        consume(tok.type);
        auto right = parseExpression(it->second.right);
        expr = std::make_unique<BinaryExpr>(std::move(expr), tok, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::parsePrimary() {
    const TokenInfo& tok = peek();
    if (tok.type == Token::INT || tok.type == Token::FLOAT) {
        consume(tok.type);
        return std::make_unique<NumberLiteral>(std::stod(tok.value));
    } else if (tok.type == Token::STRING) {
        consume(Token::STRING);
        return std::make_unique<StringLiteral>(tok.value);
    } else if (tok.type == Token::IDENTIFIER) {
        consume(Token::IDENTIFIER);
        return std::make_unique<Identifier>(tok.value);
    } else if (tok.type == Token::TRUE || tok.type == Token::FALSE || tok.type == Token::NIL) {
        consume(tok.type);
        return std::make_unique<StringLiteral>(tok.value);
    } else if (match(Token::NOT) || match(Token::MINUS) || match(Token::SHARP)) {
        TokenInfo op = consume(tok.type);
        auto operand = parsePrimary();
        return std::make_unique<UnaryExpr>(op, std::move(operand));
    } else if (match(Token::LPAREN)) {
        consume(Token::LPAREN);
        auto expr = parseExpression();
        expect(Token::RPAREN);
        return expr;
    }
    error("Expected expression");
    return nullptr;
}