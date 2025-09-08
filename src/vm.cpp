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
    stack.reserve(256);
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
                // stack[a] = LuaValue(new LuaInteger(sbx)); // This needs object implementation
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
