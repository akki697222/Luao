#include <iostream>
#include <vector>
#include "vm.hpp"
#include "opcodes.hpp"
#include "object.hpp"

using namespace luao;

#define CREATE_ABC(o, a, b, c)  ((static_cast<Instruction>(o) << 0) \
                        | (static_cast<Instruction>(a) << 7) \
                        | (static_cast<Instruction>(b) << 16) \
                        | (static_cast<Instruction>(c) << 24))

#define CREATE_ABx(o, a, bx)    ((static_cast<Instruction>(o) << 0) \
                        | (static_cast<Instruction>(a) << 7) \
                        | (static_cast<Instruction>(bx) << 15))

#define CREATE_sBx(i)   ((i) + 65535)

int main(int argc, char **argv)
{
    // local a = 1
    // local b = 2
    // local c = a + b
    // return c
    std::vector<Instruction> bytecode = {
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 0, CREATE_sBx(1)),
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 1, CREATE_sBx(2)),
        CREATE_ABC(static_cast<int>(OpCode::ADD), 2, 0, 1),
        CREATE_ABC(static_cast<int>(OpCode::RETURN), 2, 1, 0)
    };

    std::vector<LuaValue> constants = {};

    VM vm(bytecode, constants);
    vm.run();

    // TODO: Add a way to get the result from the VM and check it.
    // For now, we just check if it runs without crashing.
    std::cout << "VM execution finished." << std::endl;

    return 0;
}