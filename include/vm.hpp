#pragma once

#include "opcodes.hpp"
#include "object.hpp"
#include <vector>

namespace luao {

    class VM {
    public:
        VM();
        void run();

    private:
        Instruction* pc;
        std::vector<LuaValue> stack;
        // std::vector<LuaValue> constants;
        // Proto* proto;
    };

} // namespace luao