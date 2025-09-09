#pragma once

#include "opcodes.hpp"
#include "function.hpp"
#include <object.hpp>
#include <vector>

namespace luao {
    static LuaBool* TRUE_OBJ = new LuaBool(true);
    static LuaBool* FALSE_OBJ = new LuaBool(false);

    struct CallFrame {
        LuaFunction* func;
        const Instruction* pc;
        int stack_base;

        CallFrame(LuaFunction* func, const Instruction* pc, int stack_base)
            : func(func), pc(pc), stack_base(stack_base) {
            if (func) func->retain();
        }

        ~CallFrame() {
            if (func) func->release();
        }
    };

    class VM {
    public:
        VM();
        void load(LuaFunction* main_function);
        void run();
        LuaValue get_stack_top();
        const std::vector<LuaValue>& get_stack() const;
        void set_trace(bool trace);
        bool as_bool(const LuaValue& value);
    private:
        LuaValue main_function_;
        std::vector<CallFrame> call_stack;
        std::vector<LuaValue> stack;
        int top;
        bool trace_execution = false;
    };

} // namespace luao