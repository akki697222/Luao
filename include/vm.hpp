#pragma once

#include "opcodes.hpp"
#include "closure.hpp"
#include <object.hpp>
#include <vector>

namespace luao {
    static LuaBool* TRUE_OBJ = new LuaBool(true);
    static LuaBool* FALSE_OBJ = new LuaBool(false);

    struct CallInfo {
        LuaClosure* closure;
        const Instruction* pc;
        int stack_base;

        CallInfo(LuaClosure* closure, const Instruction* pc, int stack_base)
            : closure(closure), pc(pc), stack_base(stack_base) {
            if (closure) closure->retain();
        }

        ~CallInfo() {
            if (closure) closure->release();
        }
    };

    class VM {
    public:
        VM();
        void load(LuaClosure* main_closure);
        void run();
        LuaValue get_stack_top();
        const std::vector<LuaValue>& get_stack() const;
        void set_trace(bool trace);
        bool as_bool(const LuaValue& value);
    private:
        LuaValue main_function_;
        std::vector<CallInfo> call_stack;
        std::vector<LuaValue> stack;
        int top;
        bool trace_execution = false;
    };

} // namespace luao