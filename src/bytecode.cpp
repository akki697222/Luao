#include "bytecode.hpp"
#include "opcodes.hpp"
#include <stdexcept>

namespace luao {

// Helper to create instructions
// NOTE: These are placeholders. The real implementation needs bitwise manipulation.
Instruction create_iABC(OpCode op, int a, int b, int c) {
    return static_cast<uint32_t>(op) | (a << 8) | (b << 16) | (c << 24);
}
Instruction create_iABx(OpCode op, int a, int bx) {
    return static_cast<uint32_t>(op) | (a << 8) | (bx << 16);
}
Instruction create_iAsBx(OpCode op, int a, int sbx) {
    return create_iABx(op, a, sbx + 131071); // offset for sBx
}

// --- BytecodeGenerator ---

std::unique_ptr<FunctionProto> BytecodeGenerator::generate(const Block& ast) {
    auto main_proto = std::make_unique<FunctionProto>();
    FunctionState main_func;
    main_func.proto = main_proto.get();
    main_func.parent = nullptr;

    current_function_ = &main_func;

    visitBlock(&ast);

    // Implicit return at the end of the chunk
    emit(create_iABC(OpCode::RETURN0, 0, 0, 0));

    return main_proto;
}

void BytecodeGenerator::visit(const Statement* stmt) {
    // This is a simple dispatching visitor. A more robust solution
    // might use dynamic_cast or a virtual accept method on the AST nodes.
    if (auto s = dynamic_cast<const ExprStatement*>(stmt)) return visitExprStatement(s);
    if (auto s = dynamic_cast<const LocalStatement*>(stmt)) return visitLocalStatement(s);
    if (auto s = dynamic_cast<const ReturnStatement*>(stmt)) return visitReturnStatement(s);
    // Add other statement types here
    throw std::runtime_error("Unsupported statement type in bytecode generator.");
}

void BytecodeGenerator::visit(const Expression* expr) {
    if (auto e = dynamic_cast<const NilLiteral*>(expr)) return visitNilLiteral(e);
    if (auto e = dynamic_cast<const BoolLiteral*>(expr)) return visitBoolLiteral(e);
    if (auto e = dynamic_cast<const NumberLiteral*>(expr)) return visitNumberLiteral(e);
    if (auto e = dynamic_cast<const StringLiteral*>(expr)) return visitStringLiteral(e);
    if (auto e = dynamic_cast<const BinaryExpr*>(expr)) return visitBinaryExpr(e);
    if (auto e = dynamic_cast<const UnaryExpr*>(expr)) return visitUnaryExpr(e);
    if (auto e = dynamic_cast<const Identifier*>(expr)) return visitIdentifier(e);
    // Add other expression types here
    throw std::runtime_error("Unsupported expression type in bytecode generator.");
}

void BytecodeGenerator::visitBlock(const Block* node) {
    for (const auto& stmt : node->statements_) {
        visit(stmt.get());
    }
}

void BytecodeGenerator::visitExprStatement(const ExprStatement* node) {
    visit(node->expr_.get());
    // Result of expression is left on the stack, pop it.
    // For now, we assume expressions leave one result on the stack.
    current_function_->stack_top--;
}

void BytecodeGenerator::visitLocalStatement(const LocalStatement* node) {
    for (const auto& val : node->values_) {
        visit(val.get());
    }
    for (const auto& name : node->names_) {
        newLocal(name->name_);
    }
}

void BytecodeGenerator::visitReturnStatement(const ReturnStatement* node) {
    if (node->exprs_.empty()) {
        emit(create_iABC(OpCode::RETURN0, 0, 0, 0));
    } else {
        int first_reg = current_function_->stack_top;
        for(const auto& expr : node->exprs_) {
            visit(expr.get());
        }
        int n_returns = node->exprs_.size();
        emit(create_iABC(OpCode::RETURN, first_reg, n_returns + 1, 0));
    }
}

// --- Expression Visitors ---

void BytecodeGenerator::visitNilLiteral(const NilLiteral* node) {
    emit(create_iABC(OpCode::LOADNIL, current_function_->stack_top, 1, 0));
    current_function_->stack_top++;
}

void BytecodeGenerator::visitBoolLiteral(const BoolLiteral* node) {
    emit(create_iABC(node->value_ ? OpCode::LOADTRUE : OpCode::LOADFALSE, current_function_->stack_top, 0, 0));
    current_function_->stack_top++;
}

void BytecodeGenerator::visitNumberLiteral(const NumberLiteral* node) {
    // For simplicity, we'll try to fit it into LOADI, otherwise use LOADK
    try {
        int val = std::stoi(node->value_);
        if (val >= -131071 && val <= 131071) {
            emit(create_iAsBx(OpCode::LOADI, current_function_->stack_top, val));
            current_function_->stack_top++;
            return;
        }
    } catch (...) {
        // Not an integer or out of range, fall through to constant pool
    }

    int k_idx = addConstant(std::stod(node->value_));
    emit(create_iABx(OpCode::LOADK, current_function_->stack_top, k_idx));
    current_function_->stack_top++;
}

void BytecodeGenerator::visitStringLiteral(const StringLiteral* node) {
    int k_idx = addConstant(node->value_);
    emit(create_iABx(OpCode::LOADK, current_function_->stack_top, k_idx));
    current_function_->stack_top++;
}

void BytecodeGenerator::visitIdentifier(const Identifier* node) {
    int reg = resolveLocal(node->name_);
    if (reg != -1) {
        emit(create_iABC(OpCode::MOVE, current_function_->stack_top, reg, 0));
        current_function_->stack_top++;
    } else {
        // Assume global for now
        // This requires GETTABUP, which is more complex
        throw std::runtime_error("Global variables not yet supported.");
    }
}

void BytecodeGenerator::visitUnaryExpr(const UnaryExpr* node) {
    visit(node->operand_.get());
    OpCode op;
    switch(node->op_.type) {
        case Token::MINUS: op = OpCode::UNM; break;
        case Token::NOT: op = OpCode::NOT; break;
        case Token::LEN: op = OpCode::LEN; break;
        case Token::BNOT: op = OpCode::BNOT; break;
        default: throw std::runtime_error("Unsupported unary operator.");
    }
    // Unary ops are usually in-place, so they modify the top of the stack
    emit(create_iABC(op, current_function_->stack_top - 1, current_function_->stack_top - 1, 0));
}

void BytecodeGenerator::visitBinaryExpr(const BinaryExpr* node) {
    visit(node->left_.get());
    visit(node->right_.get());

    OpCode op;
    switch(node->op_.type) {
        case Token::PLUS: op = OpCode::ADD; break;
        case Token::MINUS: op = OpCode::SUB; break;
        case Token::MULTIPLY: op = OpCode::MUL; break;
        case Token::DIVIDE: op = OpCode::DIV; break;
        // Add other binary ops
        default: throw std::runtime_error("Unsupported binary operator.");
    }

    // Binary ops take top two stack items, pop them, and push one result.
    // The result is stored in the first register.
    int dest_reg = current_function_->stack_top - 2;
    int left_reg = current_function_->stack_top - 2;
    int right_reg = current_function_->stack_top - 1;
    emit(create_iABC(op, dest_reg, left_reg, right_reg));
    current_function_->stack_top--;
}


// --- Helpers ---

int BytecodeGenerator::emit(Instruction i) {
    current_function_->proto->code.push_back(i);
    return current_function_->proto->code.size() - 1;
}

int BytecodeGenerator::addConstant(const Value& v) {
    auto& constants = current_function_->proto->constants;
    for (size_t i = 0; i < constants.size(); ++i) {
        if (constants[i] == v) {
            return i;
        }
    }
    constants.push_back(v);
    return constants.size() - 1;
}

int BytecodeGenerator::resolveLocal(const std::string& name) {
    if (current_function_->locals.count(name)) {
        return current_function_->locals[name];
    }
    return -1; // Not found
}

int BytecodeGenerator::newLocal(const std::string& name) {
    int reg = current_function_->stack_top -1;
    current_function_->locals[name] = reg;
    return reg;
}

// Stubs for unimplemented visitors
void BytecodeGenerator::visitIfStatement(const IfStatement* node) { throw std::runtime_error("unimplemented"); }
void BytecodeGenerator::visitWhileStatement(const WhileStatement* node) { throw std::runtime_error("unimplemented"); }
void BytecodeGenerator::visitFunctionCall(const FunctionCall* node) { throw std::runtime_error("unimplemented"); }

} // namespace luao