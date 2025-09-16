#pragma once

#include "parser.hpp"
#include "opcodes.hpp"
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <variant>

namespace luao {

// Represents a constant value in the chunk
using Value = std::variant<std::nullptr_t, bool, double, std::string>;

// Represents a compiled function prototype
struct FunctionProto {
    std::vector<Instruction> code;
    std::vector<Value> constants;
    std::vector<std::unique_ptr<FunctionProto>> protos;
    int num_params = 0;
    int max_stack_size = 0;
    bool is_vararg = false;
    // Debug info
    std::vector<int> line_info;
};

// Forward declaration
class AstVisitor;

// Main class to generate bytecode from an AST
class BytecodeGenerator {
public:
    std::unique_ptr<FunctionProto> generate(const Block& ast);

private:
    // Per-function state during compilation
    struct FunctionState {
        FunctionProto* proto = nullptr;
        FunctionState* parent = nullptr;

        // Symbol table for local variables
        std::map<std::string, int> locals;
        int stack_top = 0;

        void enterScope();
        void leaveScope();
    };

    FunctionState* current_function_ = nullptr;

    void enterFunction();
    void leaveFunction();

    // Visitor methods for each AST node type
    void visit(const Statement* stmt);
    void visit(const Expression* expr);

    void visitBlock(const Block* node);
    void visitAssignStatement(const AssignStatement* node);
    void visitLocalStatement(const LocalStatement* node);
    void visitIfStatement(const IfStatement* node);
    void visitWhileStatement(const WhileStatement* node);
    void visitExprStatement(const ExprStatement* node);
    void visitReturnStatement(const ReturnStatement* node);

    void visitBinaryExpr(const BinaryExpr* node);
    void visitUnaryExpr(const UnaryExpr* node);
    void visitNumberLiteral(const NumberLiteral* node);
    void visitStringLiteral(const StringLiteral* node);
    void visitBoolLiteral(const BoolLiteral* node);
    void visitNilLiteral(const NilLiteral* node);
    void visitIdentifier(const Identifier* node);
    void visitFunctionCall(const FunctionCall* node);

    // Helpers
    int emit(Instruction i);
    int addConstant(const Value& v);
    int resolveLocal(const std::string& name);
    int newLocal(const std::string& name);
};

} // namespace luao
