/*
    Luao Lexical analyzer
*/

#include <lexer.hpp>

Lexer::Lexer(const std::string& source) : source_(source), pos_(0), line_(1) {}

TokenInfo Lexer::readLongString() {
    int start_line = line_;
    advance(); // consume first '['

    int level = 0;
    while(peekChar() == '=') {
        level++;
        advance();
    }

    if (peekChar() != '[') {
        throwError("invalid long string delimiter");
    }
    advance(); // consume second '['

    // If the next char is a newline, skip it
    if (peekChar() == '\n') {
        advance();
    }

    std::string value;
    while (pos_ < source_.size()) {
        if (peekChar() == ']') {
            size_t saved_pos = pos_;
            int saved_line = line_;
            advance(); // consume ']'

            int close_level = 0;
            while(peekChar() == '=') {
                close_level++;
                advance();
            }

            if (peekChar() == ']' && close_level == level) {
                advance(); // consume final ']'
                return {Token::STRING, value, start_line};
            }

            pos_ = saved_pos;
            line_ = saved_line;
        }

        value += peekChar();
        advance();
    }

    throwError("unfinished long string starting at line " + std::to_string(start_line));
    return {Token::EOS, "", line_}; // Unreachable
}


TokenInfo Lexer::nextToken() {
    skipWhitespace();
    if (pos_ >= source_.size()) {
        return {Token::EOS, "", line_};
    }

    char current = source_[pos_];

    switch (current) {
        case '+': advance(); return {Token::PLUS, "+", line_};
        case '-': advance(); return {Token::MINUS, "-", line_};
        case '*': advance(); return {Token::MULTIPLY, "*", line_};
        case '/':
            advance();
            if (peekChar() == '/') { advance(); return {Token::IDIV, "//", line_}; }
            return {Token::DIVIDE, "/", line_};
        case '%': advance(); return {Token::MODULO, "%", line_};
        case '^': advance(); return {Token::POW, "^", line_};
        case '#': advance(); return {Token::LEN, "#", line_};
        case '&': advance(); return {Token::BAND, "&", line_};
        case '|': advance(); return {Token::BOR, "|", line_};
        case '=':
            advance();
            if (peekChar() == '=') { advance(); return {Token::EQ, "==", line_}; }
            return {Token::ASSIGN, "=", line_};
        case '~':
            advance();
            if (peekChar() == '=') { advance(); return {Token::NE, "~=", line_}; }
            return {Token::BNOT, "~", line_};
        case '<':
            advance();
            if (peekChar() == '=') { advance(); return {Token::LE, "<=", line_}; }
            if (peekChar() == '<') { advance(); return {Token::SHL, "<<", line_}; }
            return {Token::LT, "<", line_};
        case '>':
            advance();
            if (peekChar() == '=') { advance(); return {Token::GE, ">=", line_}; }
            if (peekChar() == '>') { advance(); return {Token::SHR, ">>", line_}; }
            return {Token::GT, ">", line_};
        case '(': advance(); return {Token::LPAREN, "(", line_};
        case ')': advance(); return {Token::RPAREN, ")", line_};
        case '{': advance(); return {Token::LBRACE, "{", line_};
        case '}': advance(); return {Token::RBRACE, "}", line_};
        case '[':
            if (pos_ + 1 < source_.size() && (source_[pos_ + 1] == '[' || source_[pos_ + 1] == '=')) {
                return readLongString();
            }
            advance();
            return {Token::LBRACKET, "[", line_};
        case ']': advance(); return {Token::RBRACKET, "]", line_};
        case ';': advance(); return {Token::SEMICOLON, ";", line_};
        case ':':
            advance();
            if (peekChar() == ':') { advance(); return {Token::COLON_DB, "::", line_}; }
            return {Token::COLON, ":", line_};
        case ',': advance(); return {Token::COMMA, ",", line_};
        case '.':
            advance();
            if (peekChar() == '.') {
                advance();
                if (peekChar() == '.') { advance(); return {Token::VARARG, "...", line_}; }
                return {Token::CONCAT, "..", line_};
            }
            return {Token::DOT, ".", line_};
    }

    if (isAlpha(current) || current == '_') {
        return readIdentifierOrKeyword();
    }

    if (isDigit(current)) {
        return readNumber();
    }

    if (current == '"' || current == '\'') {
        return readString();
    }

    throwError("unexpected symbol near '" + std::string(1, current) + "'");
    return {Token::EOS, "", line_};
}

TokenInfo Lexer::peek() {
    size_t saved_pos = pos_;
    int saved_line = line_;
    TokenInfo tok = nextToken();
    pos_ = saved_pos;
    line_ = saved_line;
    return tok;
}

void Lexer::advance() {
    if (pos_ < source_.size()) {
        if (source_[pos_] == '\n') line_++;
        pos_++;
    }
}

char Lexer::peekChar() const {
    return pos_ < source_.size() ? source_[pos_] : '\0';
}

void Lexer::skipWhitespace() {
    while (pos_ < source_.size()) {
        char c = source_[pos_];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v') {
            advance();
        } else if (c == '-') {
            if (pos_ + 1 < source_.size() && source_[pos_ + 1] == '-') {
                skipComment();
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

void Lexer::skipComment() {
    advance(); // -
    advance(); // -

    if (peekChar() == '[') {
        advance(); // [
        int level = 0;
        while(peekChar() == '=') {
            level++;
            advance();
        }
        if (peekChar() == '[') {
            advance(); // [
            // Long comment
            while (pos_ < source_.size()) {
                if (peekChar() == ']') {
                    advance();
                    int close_level = 0;
                    while(peekChar() == '=') {
                        close_level++;
                        advance();
                    }
                    if (peekChar() == ']' && close_level == level) {
                        advance();
                        return;
                    }
                } else {
                    advance();
                }
            }
            throwError("unfinished long comment");
        }
    }

    // Short comment
    while (pos_ < source_.size() && source_[pos_] != '\n') {
        advance();
    }
}

// This function is now unused, but I'll keep it for reference
void Lexer::skipLongComment() {
    int level = 1;
    while (pos_ < source_.size()) {
        if (source_[pos_] == '[' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '[') {
            advance();
            advance();
            level++;
        } else if (source_[pos_] == ']' && pos_ + 1 < source_.size() && source_[pos_ + 1] == ']') {
            advance();
            advance();
            if (--level == 0) return;
        } else {
            advance();
        }
    }
    throwError("unfinished long comment");
}

TokenInfo Lexer::readIdentifierOrKeyword() {
    std::string value;
    while (pos_ < source_.size() && (isAlpha(source_[pos_]) || isDigit(source_[pos_]) || source_[pos_] == '_')) {
        value += source_[pos_];
        advance();
    }
    if (value == "and") return {Token::AND, value, line_};
    if (value == "break") return {Token::BREAK, value, line_};
    if (value == "do") return {Token::DO, value, line_};
    if (value == "else") return {Token::ELSE, value, line_};
    if (value == "elseif") return {Token::ELSEIF, value, line_};
    if (value == "end") return {Token::END, value, line_};
    if (value == "false") return {Token::FALSE, value, line_};
    if (value == "for") return {Token::FOR, value, line_};
    if (value == "function") return {Token::FUNCTION, value, line_};
    if (value == "goto") return {Token::GOTO, value, line_};
    if (value == "if") return {Token::IF, value, line_};
    if (value == "in") return {Token::IN, value, line_};
    if (value == "local") return {Token::LOCAL, value, line_};
    if (value == "nil") return {Token::NIL, value, line_};
    if (value == "not") return {Token::NOT, value, line_};
    if (value == "or") return {Token::OR, value, line_};
    if (value == "repeat") return {Token::REPEAT, value, line_};
    if (value == "return") return {Token::RETURN, value, line_};
    if (value == "then") return {Token::THEN, value, line_};
    if (value == "true") return {Token::TRUE, value, line_};
    if (value == "until") return {Token::UNTIL, value, line_};
    if (value == "while") return {Token::WHILE, value, line_};
    return {Token::IDENTIFIER, value, line_};
}

TokenInfo Lexer::readNumber() {
    std::string value;
    bool isFloat = false;

    // Hexadecimal
    if (source_[pos_] == '0' && pos_ + 1 < source_.size() && 
        (source_[pos_ + 1] == 'x' || source_[pos_ + 1] == 'X')) {
        value += source_[pos_++]; // '0'
        value += source_[pos_++]; // 'x' or 'X'
        
        if (pos_ >= source_.size() || !isHexDigit(source_[pos_])) {
            throwError("malformed number near '" + value + "'");
        }
        
        while (pos_ < source_.size() && isHexDigit(source_[pos_])) {
            value += source_[pos_];
            advance();
        }
        // TODO: Hex floats with 'p' or 'P'
        return {Token::INT, value, line_};
    }
    
    while (pos_ < source_.size() && (isDigit(source_[pos_]) || source_[pos_] == '.')) {
        if (source_[pos_] == '.') {
            if (isFloat) break;
            if (pos_ + 1 < source_.size() && source_[pos_ + 1] == '.') {
                break;
            }
            isFloat = true;
        }
        value += source_[pos_];
        advance();
    }
    if (pos_ < source_.size() && (source_[pos_] == 'e' || source_[pos_] == 'E')) {
        isFloat = true;
        value += source_[pos_];
        advance();
        
        if (pos_ < source_.size() && (source_[pos_] == '+' || source_[pos_] == '-')) {
            value += source_[pos_];
            advance();
        }
        
        if (pos_ >= source_.size() || !isDigit(source_[pos_])) {
            throwError("malformed number near '" + value + "'");
        }
        
        while (pos_ < source_.size() && isDigit(source_[pos_])) {
            value += source_[pos_];
            advance();
        }
    }
    
    return {isFloat ? Token::FLOAT : Token::INT, value, line_};
}

TokenInfo Lexer::readString() {
    std::string value;
    char delimiter = source_[pos_];
    advance(); // " or '
    int start_line = line_;
    
    while (pos_ < source_.size() && source_[pos_] != delimiter) {
        if (source_[pos_] == '\n') {
            throwError("unfinished string near '" + std::string(1, delimiter) + "'");
        }
        if (source_[pos_] == '\\') {
            advance();
            if (pos_ >= source_.size()) {
                throwError("unfinished string near '" + std::string(1, delimiter) + "'");
            }
            char c = source_[pos_];
            switch (c) {
                case 'a': value += '\a'; break;
                case 'b': value += '\b'; break;
                case 'f': value += '\f'; break;
                case 'n': value += '\n'; break;
                case 'r': value += '\r'; break;
                case 't': value += '\t'; break;
                case 'v': value += '\v'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '\'': value += '\''; break;
                case '\n': value += '\n'; break;
                case 'z':
                    advance();
                    while (pos_ < source_.size() && 
                           (source_[pos_] == ' ' || source_[pos_] == '\t' || 
                            source_[pos_] == '\r' || source_[pos_] == '\n')) {
                        advance();
                    }
                    pos_--;
                    break;
                default:
                    if (isDigit(c)) {
                        int num = 0;
                        for (int i = 0; i < 3 && pos_ < source_.size() && isDigit(source_[pos_]); i++) {
                            num = num * 10 + (source_[pos_] - '0');
                            if (num > 255) {
                                throwError("decimal escape too large near '" + 
                                         std::string(1, '\\') + std::to_string(num) + "'");
                            }
                            advance();
                        }
                        value += static_cast<char>(num);
                        pos_--;
                    } else if (c == 'x') {
                        advance();
                        int num = 0;
                        if (pos_ + 1 < source_.size() && isHexDigit(source_[pos_]) && isHexDigit(source_[pos_ + 1])) {
                            // Simplified: doesn't handle single-digit hex like \xa
                            num = (isDigit(source_[pos_]) ? source_[pos_] - '0' : (tolower(source_[pos_]) - 'a' + 10)) * 16;
                            advance();
                            num += (isDigit(source_[pos_]) ? source_[pos_] - '0' : (tolower(source_[pos_]) - 'a' + 10));
                            value += static_cast<char>(num);
                        } else {
                            throwError("hexadecimal escape sequence must have 2 digits");
                        }
                    }
                    else {
                        throwError("invalid escape sequence near '\\" + std::string(1, c) + "'");
                    }
                    break;
            }
            advance();
        } else {
            value += source_[pos_];
            advance();
        }
    }
    
    if (pos_ >= source_.size()) {
        if (start_line == line_) {
            throwError("unfinished string near '" + std::string(1, delimiter) + "'");
        } else {
            throwError("unfinished string near '<eof>'");
        }
    }
    
    advance();
    return {Token::STRING, value, line_};
}

void Lexer::throwError(const std::string& msg) {
    throw std::runtime_error("luaoc: stdin:" + std::to_string(line_) + ": " + msg);
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isHexDigit(char c) const {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}