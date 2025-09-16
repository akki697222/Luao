#include "parser.hpp"
#include <stdexcept>

// Operator precedence and associativity
// Pair: {left precedence, right precedence}
static const std::map<Token, std::pair<int, int>> PRECEDENCE_MAP = {
    {Token::OR,       {1, 1}},
    {Token::AND,      {2, 2}},
    {Token::LT,       {3, 3}}, {Token::GT, {3, 3}}, {Token::LE, {3, 3}}, {Token::GE, {3, 3}}, {Token::NE, {3, 3}}, {Token::EQ, {3, 3}},
    {Token::BOR,      {4, 4}},
    {Token::BXOR,     {5, 5}},
    {Token::BAND,     {6, 6}},
    {Token::SHL,      {7, 7}}, {Token::SHR, {7, 7}},
    {Token::CONCAT,   {9, 8}}, // Right associative
    {Token::PLUS,     {10, 10}}, {Token::MINUS, {10, 10}},
    {Token::MULTIPLY, {11, 11}}, {Token::DIVIDE, {11, 11}}, {Token::IDIV, {11, 11}}, {Token::MODULO, {11, 11}},
    {Token::POW,      {13, 12}}, // Right associative
};
const int UNARY_PRECEDENCE = 12;

// --- Parser Core ---

Parser::Parser(const std::string& source) : lexer_(source) {
    advance();
}

void Parser::advance() {
    current_token_ = lexer_.nextToken();
}

void Parser::consume(Token type) {
    if (current_token_.type == type) {
        advance();
    } else {
        error("Expected token " + std::string(TokenNames[static_cast<int>(type)]) +
              " but got " + current_token_.value);
    }
}

bool Parser::match(Token type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(Token type) {
    return current_token_.type == type;
}

void Parser::error(const std::string& message) {
    throw std::runtime_error("Parser Error at line " + std::to_string(current_token_.line) + ": " + message);
}

std::unique_ptr<Block> Parser::parse() {
    auto block = std::make_unique<Block>();
    block->statements_ = parseStatementList();
    if (!check(Token::EOS)) {
        error("Expected <eof> at end of chunk.");
    }
    return block;
}

// --- Statement Parsers ---

std::vector<std::unique_ptr<Statement>> Parser::parseStatementList() {
    std::vector<std::unique_ptr<Statement>> stmts;
    while (!check(Token::END) && !check(Token::ELSE) && !check(Token::ELSEIF) && !check(Token::UNTIL) && !check(Token::EOS)) {
        if (check(Token::RETURN)) {
            stmts.push_back(parseReturnStatement());
            break;
        }
        auto stmt = parseStatement();
        if (match(Token::SEMICOLON)) {
            // optional semicolon
        }
        if (stmt) {
            stmts.push_back(std::move(stmt));
        }
    }
    return stmts;
}

std::unique_ptr<Block> Parser::parseBlock() {
    auto block = std::make_unique<Block>();
    block->statements_ = parseStatementList();
    return block;
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (check(Token::IF)) return parseIfStatement();
    if (check(Token::WHILE)) return parseWhileStatement();
    if (check(Token::DO)) return parseDoStatement();
    if (check(Token::FOR)) return parseForStatement();
    if (check(Token::REPEAT)) return parseRepeatStatement();
    if (check(Token::FUNCTION)) return parseFunctionStatement();
    if (check(Token::LOCAL)) return parseLocalStatement();
    if (check(Token::GOTO)) return parseGotoStatement();
    if (check(Token::DBCOLON)) return parseLabelStatement();
    if (check(Token::BREAK)) return parseBreakStatement();
    if (check(Token::RETURN)) return parseReturnStatement();
    return parseAssignOrCallStatement();
}

std::unique_ptr<Statement> Parser::parseLocalStatement() {
    consume(Token::LOCAL);
    if (match(Token::FUNCTION)) {
        auto func_stmt = std::make_unique<FunctionStatement>();
        func_stmt->is_local_ = true;
        func_stmt->name_ = parseIdentifier(false);
        func_stmt->def_ = parseFunctionDef();
        return func_stmt;
    }

    auto local_stmt = std::make_unique<LocalStatement>();
    do {
        local_stmt->names_.push_back(parseIdentifier(true));
    } while (match(Token::COMMA));

    if (match(Token::ASSIGN)) {
        do {
            local_stmt->values_.push_back(parseExpression());
        } while (match(Token::COMMA));
    }
    return local_stmt;
}

std::unique_ptr<Statement> Parser::parseAssignOrCallStatement() {
    auto prefix_expr = parseSuffixedExpression();

    // If it's a function call, it's a statement
    if (dynamic_cast<FunctionCall*>(prefix_expr.get())) {
        return std::make_unique<ExprStatement>(std::move(prefix_expr));
    }

    // Otherwise, it must be an assignment
    auto assign_stmt = std::make_unique<AssignStatement>();
    assign_stmt->targets_.push_back(std::move(prefix_expr));

    while (match(Token::COMMA)) {
        assign_stmt->targets_.push_back(parseSuffixedExpression());
    }

    consume(Token::ASSIGN);

    do {
        assign_stmt->values_.push_back(parseExpression());
    } while (match(Token::COMMA));

    return assign_stmt;
}

std::unique_ptr<Statement> Parser::parseIfStatement() {
    auto if_stmt = std::make_unique<IfStatement>();

    auto parse_clause = [this]() {
        IfClause clause;
        clause.condition = parseExpression();
        consume(Token::THEN);
        clause.body = parseBlock();
        return clause;
    };

    consume(Token::IF);
    if_stmt->if_clauses_.push_back(parse_clause());

    while (match(Token::ELSEIF)) {
        if_stmt->if_clauses_.push_back(parse_clause());
    }

    if (match(Token::ELSE)) {
        if_stmt->else_body_ = parseBlock();
    }

    consume(Token::END);
    return if_stmt;
}

std::unique_ptr<Statement> Parser::parseWhileStatement() {
    consume(Token::WHILE);
    auto cond = parseExpression();
    consume(Token::DO);
    auto body = parseBlock();
    consume(Token::END);
    auto while_stmt = std::make_unique<WhileStatement>();
    while_stmt->condition_ = std::move(cond);
    while_stmt->body_ = std::move(body);
    return while_stmt;
}

// --- Expression Parsers ---

std::unique_ptr<Expression> Parser::parseExpression(int precedence) {
    auto left = parsePrefixExpression();

    while (true) {
        auto it = PRECEDENCE_MAP.find(current_token_.type);
        if (it == PRECEDENCE_MAP.end() || it->second.first < precedence) {
            break;
        }

        TokenInfo op = current_token_;
        advance();

        auto right = parseExpression(it->second.second);
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }
    return left;
}

std::unique_ptr<Expression> Parser::parsePrefixExpression() {
    if (check(Token::MINUS) || check(Token::NOT) || check(Token::LEN) || check(Token::BNOT)) {
        TokenInfo op = current_token_;
        advance();
        auto operand = parseExpression(UNARY_PRECEDENCE);
        return std::make_unique<UnaryExpr>(op, std::move(operand));
    }
    return parseSuffixedExpression();
}

std::unique_ptr<Expression> Parser::parseSuffixedExpression() {
    auto expr = parseSimpleExpression();

    while (true) {
        if (match(Token::DOT)) {
            auto field = parseIdentifier(false);
            expr = std::make_unique<TableAccess>(std::move(expr), std::move(field));
        } else if (match(Token::LBRACKET)) {
            auto index = parseExpression();
            consume(Token::RBRACKET);
            expr = std::make_unique<IndexAccess>(std::move(expr), std::move(index));
        } else if (check(Token::LPAREN) || check(Token::LBRACE) || check(Token::STRING)) {
            auto call_expr = std::make_unique<FunctionCall>(std::move(expr), std::vector<std::unique_ptr<Expression>>{});
            parseFunctionArgs(call_expr->args_);
            expr = std::move(call_expr);
        } else if (match(Token::COLON)) {
            auto call_expr = std::make_unique<FunctionCall>(std::move(expr), std::vector<std::unique_ptr<Expression>>{});
            call_expr->method_name_ = std::make_unique<StringLiteral>(parseIdentifier(false)->name_);
            parseFunctionArgs(call_expr->args_);
            expr = std::move(call_expr);
        }
        else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expression> Parser::parseSimpleExpression() {
    if (match(Token::NIL)) return std::make_unique<NilLiteral>();
    if (match(Token::TRUE)) return std::make_unique<BoolLiteral>(true);
    if (match(Token::FALSE)) return std::make_unique<BoolLiteral>(false);
    if (check(Token::INT) || check(Token::FLOAT)) {
        auto token = current_token_;
        advance();
        return std::make_unique<NumberLiteral>(token.value);
    }
    if (check(Token::STRING)) {
        auto token = current_token_;
        advance();
        return std::make_unique<StringLiteral>(token.value);
    }
    if (match(Token::VARARG)) return std::make_unique<VarargLiteral>();
    if (check(Token::IDENTIFIER)) return parseIdentifier(false);
    if (match(Token::LPAREN)) {
        auto expr = parseExpression();
        consume(Token::RPAREN);
        return expr;
    }
    if (check(Token::FUNCTION)) return parseFunctionDef();
    if (check(Token::LBRACE)) return parseTableConstructor();

    error("Unexpected token in expression: " + current_token_.value);
    return nullptr;
}


std::unique_ptr<Identifier> Parser::parseIdentifier(bool can_have_attr) {
    if (!check(Token::IDENTIFIER)) {
        error("Expected an identifier.");
    }
    std::string name = current_token_.value;
    advance();

    std::string attr = "";
    if (can_have_attr && match(Token::LT)) {
        attr = parseAttribute();
    }
    return std::make_unique<Identifier>(name, attr);
}

std::string Parser::parseAttribute() {
    std::string attr_val;
    // This is a simplified attribute parser. It just reads identifiers.
    if (check(Token::IDENTIFIER)) {
        attr_val = current_token_.value;
        advance();
    }
    consume(Token::GT);
    return attr_val;
}

void Parser::parseFunctionArgs(std::vector<std::unique_ptr<Expression>>& args) {
    if (match(Token::LPAREN)) {
        if (!check(Token::RPAREN)) {
            do {
                args.push_back(parseExpression());
            } while (match(Token::COMMA));
        }
        consume(Token::RPAREN);
    } else if (check(Token::LBRACE)) {
        args.push_back(parseTableConstructor());
    } else {
        consume(Token::STRING);
        args.push_back(std::make_unique<StringLiteral>(current_token_.value));
    }
}

std::unique_ptr<FunctionDef> Parser::parseFunctionDef() {
    consume(Token::FUNCTION);
    auto def = std::make_unique<FunctionDef>();
    consume(Token::LPAREN);
    if (!check(Token::RPAREN)) {
        do {
            if (match(Token::VARARG)) {
                def->is_vararg_ = true;
                break;
            }
            def->params_.push_back(parseIdentifier(true));
        } while (match(Token::COMMA));
    }
    consume(Token::RPAREN);
    def->body_ = parseBlock();
    consume(Token::END);
    return def;
}

std::unique_ptr<Statement> Parser::parseReturnStatement() {
    consume(Token::RETURN);
    auto ret_stmt = std::make_unique<ReturnStatement>();
    if (!check(Token::END) && !check(Token::SEMICOLON) && !check(Token::EOS)) {
        do {
            ret_stmt->exprs_.push_back(parseExpression());
        } while (match(Token::COMMA));
    }
    return ret_stmt;
}

std::unique_ptr<TableConstructor> Parser::parseTableConstructor() {
    auto table = std::make_unique<TableConstructor>();
    consume(Token::LBRACE);

    while (!match(Token::RBRACE)) {
        TableField field;
        if (match(Token::LBRACKET)) {
            field.key = parseExpression();
            consume(Token::RBRACKET);
            consume(Token::ASSIGN);
            field.value = parseExpression();
        } else if (check(Token::IDENTIFIER)) {
            // could be `key = value` or just `value`
            auto id = parseIdentifier(false);
            if (match(Token::ASSIGN)) {
                field.key = std::make_unique<StringLiteral>(id->name_);
                field.value = parseExpression();
            } else {
                field.key = nullptr; // List-style
                field.value = std::move(id);
            }
        } else {
            field.key = nullptr; // List-style
            field.value = parseExpression();
        }
        table->fields_.push_back(std::move(field));

        if (!match(Token::COMMA) && !match(Token::SEMICOLON)) {
            break;
        }
    }
    consume(Token::RBRACE);
    return table;
}


// --- Stubs for remaining statements ---
std::unique_ptr<Statement> Parser::parseDoStatement() { error("unimplemented: parseDoStatement"); return nullptr; }
std::unique_ptr<Statement> Parser::parseForStatement() { error("unimplemented: parseForStatement"); return nullptr; }
std::unique_ptr<Statement> Parser::parseRepeatStatement() { error("unimplemented: parseRepeatStatement"); return nullptr; }
std::unique_ptr<Statement> Parser::parseFunctionStatement() { error("unimplemented: parseFunctionStatement"); return nullptr; }
std::unique_ptr<Statement> Parser::parseGotoStatement() { error("unimplemented: parseGotoStatement"); return nullptr; }
std::unique_ptr<Statement> Parser::parseLabelStatement() { error("unimplemented: parseLabelStatement"); return nullptr; }
std::unique_ptr<Statement> Parser::parseBreakStatement() { consume(Token::BREAK); return std::make_unique<BreakStatement>(); }