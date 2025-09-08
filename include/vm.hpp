#pragma once

#include "opcodes.hpp"
#include "object.hpp"
#include <vector>

namespace luao {

    class VM {
    public:
        VM();
        VM(std::vector<Instruction> bytecode, std::vector<LuaValue> constants);
        void load(std::vector<Instruction> bytecode, std::vector<LuaValue> constants);
        void run();

    private:
        std::vector<Instruction> bytecode;
        Instruction* pc;
        std::vector<LuaValue> stack;
        std::vector<LuaValue> constants;
    };

} // namespace luao