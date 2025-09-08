#include "vm.hpp"
#include "debug.hpp"
#include <iostream>

namespace luao {

#define GET_OPCODE(i)   (static_cast<OpCode>(((i) >> 0) & 0x7F))
#define GETARG_A(i)     (((i) >> 7) & 0xFF)
#define GETARG_B(i)     (((i) >> 16) & 0xFF)
#define GETARG_C(i)     (((i) >> 24) & 0xFF)
#define GETARG_Bx(i)    ((i) >> 15)
#define GETARG_sBx(i)   (static_cast<int>(GETARG_Bx(i)) - 65535)


VM::VM() : pc(nullptr), top(0) {
    stack.resize(256);
}

VM::VM(std::vector<Instruction> bytecode, std::vector<LuaValue> constants) : top(0) {
    load(std::move(bytecode), std::move(constants));
}

void VM::load(std::vector<Instruction> bytecode, std::vector<LuaValue> constants) {
    this->bytecode = std::move(bytecode);
    this->constants = std::move(constants);
    this->pc = &this->bytecode[0];
    stack.clear();
    stack.resize(256);
    this->top = 0;
}

LuaValue VM::get_stack_top() {
    if (top > 0) {
        return stack[top - 1];
    }
    return LuaValue(); // Return nil if stack is empty
}

const std::vector<LuaValue>& VM::get_stack() const {
    return stack;
}

void VM::set_trace(bool trace) {
    trace_execution = trace;
}

static luaNumber get_number_from_value(const LuaValue& val) {
    if (val.getType() == LuaType::NUMBER) {
        if (auto* num = dynamic_cast<const LuaNumber*>(val.getObject())) {
            return num->getValue();
        }
        if (auto* integer = dynamic_cast<const LuaInteger*>(val.getObject())) {
            return static_cast<luaNumber>(integer->getValue());
        }
    }
    return 0.0;
}

void VM::run() {
    for (;;) {
        Instruction i = *pc;
        if (trace_execution) {
            std::cout << disassemble_instruction(i) << std::endl;
        }
        pc++;
        OpCode op = GET_OPCODE(i);

        switch (op) {
            case OpCode::MOVE: {
                int a = GETARG_A(i);
                int b = GETARG_B(i);
                stack[a] = stack[b];
                break;
            }
            case OpCode::LOADI: {
                int a = GETARG_A(i);
                int sbx = GETARG_sBx(i);
                stack[a] = LuaValue(new LuaInteger(sbx), LuaType::NUMBER);
                top = a + 1;
                break;
            }
            case OpCode::LOADK: {
                int a = GETARG_A(i);
                int bx = GETARG_Bx(i);
                stack[a] = constants[bx];
                top = a + 1;
                break;
            }
            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::DIV: {
                int a = GETARG_A(i);
                int b = GETARG_B(i);
                int c = GETARG_C(i);
                LuaValue rb = stack[b];
                LuaValue rc = stack[c];

                if (rb.getType() == LuaType::NUMBER && rc.getType() == LuaType::NUMBER) {
                    auto* int_b = dynamic_cast<LuaInteger*>(rb.getObject());
                    auto* int_c = dynamic_cast<LuaInteger*>(rc.getObject());

                    if (int_b && int_c && op != OpCode::DIV) {
                        luaInt val_b = int_b->getValue();
                        luaInt val_c = int_c->getValue();
                        luaInt res;
                        if (op == OpCode::ADD) res = val_b + val_c;
                        else if (op == OpCode::SUB) res = val_b - val_c;
                        else res = val_b * val_c;
                        stack[a] = LuaValue(new LuaInteger(res), LuaType::NUMBER);
                    } else {
                        luaNumber nb = get_number_from_value(rb);
                        luaNumber nc = get_number_from_value(rc);
                        luaNumber res;
                        if (op == OpCode::ADD) res = nb + nc;
                        else if (op == OpCode::SUB) res = nb - nc;
                        else if (op == OpCode::MUL) res = nb * nc;
                        else res = nb / nc;
                        stack[a] = LuaValue(new LuaNumber(res), LuaType::NUMBER);
                    }
                } else {
                    // Metamethods would be handled here
                }
                top = a + 1;
                break;
            }
            case OpCode::RETURN: {
                int a = GETARG_A(i);
                int b = GETARG_B(i);
                if (b == 1) { // 0 results
                    top = 0;
                } else if (b > 1) { // b-1 results
                    for (int j = 0; j < b - 1; j++) {
                        stack[j] = stack[a + j];
                    }
                    top = b - 1;
                }
                return;
            }
            case OpCode::RETURN1: {
                int a = GETARG_A(i);
                stack[0] = stack[a];
                top = 1;
                return;
            }
            default: {
                std::cout << "Unknown opcode: " << to_string(op) << std::endl;
                return;
            }
        }
    }
}

} // namespace luao
