#include "vm.hpp"
#include <iostream>

namespace luao {

#define GET_OPCODE(i)   (static_cast<OpCode>(((i) >> 0) & 0x7F))
#define GETARG_A(i)     (((i) >> 7) & 0xFF)
#define GETARG_B(i)     (((i) >> 16) & 0xFF)
#define GETARG_C(i)     (((i) >> 24) & 0xFF)
#define GETARG_Bx(i)    ((i) >> 15)
#define GETARG_sBx(i)   (static_cast<int>(GETARG_Bx(i)) - 65535)


VM::VM() : pc(nullptr) {
    stack.resize(256);
}

VM::VM(std::vector<Instruction> bytecode, std::vector<LuaValue> constants) {
    load(std::move(bytecode), std::move(constants));
}

void VM::load(std::vector<Instruction> bytecode, std::vector<LuaValue> constants) {
    this->bytecode = std::move(bytecode);
    this->constants = std::move(constants);
    this->pc = &this->bytecode[0];
    stack.clear();
    stack.resize(256);
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
        Instruction i = *pc++;
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
                break;
            }
            case OpCode::LOADK: {
                int a = GETARG_A(i);
                int bx = GETARG_Bx(i);
                stack[a] = constants[bx];
                break;
            }
            case OpCode::ADD: {
                int a = GETARG_A(i);
                int b = GETARG_B(i);
                int c = GETARG_C(i);
                LuaValue rb = stack[b];
                LuaValue rc = stack[c];

                if (rb.getType() == LuaType::NUMBER && rc.getType() == LuaType::NUMBER) {
                    auto* int_b = dynamic_cast<LuaInteger*>(rb.getObject());
                    auto* int_c = dynamic_cast<LuaInteger*>(rc.getObject());

                    if (int_b && int_c) {
                        stack[a] = LuaValue(new LuaInteger(int_b->getValue() + int_c->getValue()), LuaType::NUMBER);
                    } else {
                        luaNumber nb = get_number_from_value(rb);
                        luaNumber nc = get_number_from_value(rc);
                        stack[a] = LuaValue(new LuaNumber(nb + nc), LuaType::NUMBER);
                    }
                } else {
                    // Metamethods would be handled here
                }
                break;
            }
            case OpCode::RETURN: {
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
