#pragma once

#include "opcodes.hpp"
#include <object.hpp>
#include <vector>

namespace luao {
    static LuaBool* TRUE_OBJ = new LuaBool(true);
    static LuaBool* FALSE_OBJ = new LuaBool(false);

    class VM {
    public:
        VM();
        VM(std::vector<Instruction> bytecode, std::vector<LuaValue> constants);
        void load(std::vector<Instruction> bytecode, std::vector<LuaValue> constants);
        void run();
        LuaValue get_stack_top();
        const std::vector<LuaValue>& get_stack() const;
        void set_trace(bool trace);
        bool as_bool(const LuaValue& value);
    private:
        std::vector<Instruction> bytecode;
        Instruction* pc;
        std::vector<LuaValue> stack;
        int top;
        std::vector<LuaValue> constants;
        bool trace_execution = false;
    };

} // namespace luao