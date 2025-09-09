#include "debug.hpp"
#include "opcodes.hpp"
#include <sstream>

namespace luao {

// These macros are also defined in vm.cpp. It would be better to move them
// to a common header, but for now, I will just redefine them here.
#define GET_OPCODE(i)   (static_cast<OpCode>(((i) >> 0) & 0x7F))
#define GETARG_A(i)     (((i) >> 7) & 0xFF)
#define GETARG_B(i)     (((i) >> 16) & 0xFF)
#define GETARG_C(i)     (((i) >> 24) & 0xFF)
#define GETARG_Bx(i)    ((i) >> 15)
#define GETARG_sBx(i)   (static_cast<int>(GETARG_Bx(i)) - 65535)

std::string disassemble_instruction(Instruction i, const LuaFunction* func) {
    std::stringstream ss;
    OpCode op = GET_OPCODE(i);
    ss << to_string(op) << " ";

    switch (op) {
        case OpCode::MOVE:
        case OpCode::ADD:
        case OpCode::SUB:
        case OpCode::MUL:
        case OpCode::DIV:
        case OpCode::GETTABLE:
        case OpCode::SETTABLE:
            ss << GETARG_A(i) << " " << GETARG_B(i) << " " << GETARG_C(i);
            break;
        case OpCode::LOADI:
            ss << GETARG_A(i) << " " << GETARG_sBx(i);
            break;
        case OpCode::LOADK:
            ss << GETARG_A(i) << " " << GETARG_Bx(i);
            if (func) {
                ss << " (" << func->getConstants()[GETARG_Bx(i)].getObject()->toString() << ")";
            }
            break;
        case OpCode::RETURN:
        case OpCode::RETURN1:
            ss << GETARG_A(i) << " " << GETARG_B(i);
            break;
        default:
            // For other opcodes, just print A, B, C for now
            ss << GETARG_A(i) << " " << GETARG_B(i) << " " << GETARG_C(i);
            break;
    }
    return ss.str();
}

} // namespace luao
