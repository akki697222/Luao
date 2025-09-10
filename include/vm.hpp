#pragma once

#include <opcodes.hpp>
#include <closure.hpp>
#include <object.hpp>
#include <vector>

#define CRITICAL_DUMP_CONTEXT_LINES 5

namespace luao {
    static LuaBool* TRUE_OBJ = new LuaBool(true);
    static LuaBool* FALSE_OBJ = new LuaBool(false);

    struct CallInfo;
    class VM;

    void dump_critical_error(const VM& vm, std::string err, CallInfo* frame, const Instruction* current_pc);

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
        std::vector<LuaValue>& get_stack_mutable();
        void set_trace(bool trace);
        bool as_bool(const LuaValue& value);
        const std::vector<CallInfo>& get_call_stack() const;
        const CallInfo& get_call_stack_top() const;
        const Instruction* get_current_pc();
        
        // Arithmetic operations with metamethod support
        LuaValue add(const LuaValue& a, const LuaValue& b);
        LuaValue sub(const LuaValue& a, const LuaValue& b);
        LuaValue mul(const LuaValue& a, const LuaValue& b);
        LuaValue div(const LuaValue& a, const LuaValue& b);
        LuaValue mod(const LuaValue& a, const LuaValue& b);
        LuaValue pow(const LuaValue& a, const LuaValue& b);
        LuaValue idiv(const LuaValue& a, const LuaValue& b);
        LuaValue unm(const LuaValue& a);
        LuaValue len(const LuaValue& a);
        LuaValue concat(const LuaValue& a, const LuaValue& b);
        
        // Bitwise operations with metamethod support
        LuaValue band(const LuaValue& a, const LuaValue& b);
        LuaValue bor(const LuaValue& a, const LuaValue& b);
        LuaValue bxor(const LuaValue& a, const LuaValue& b);
        LuaValue bnot(const LuaValue& a);
        LuaValue shl(const LuaValue& a, const LuaValue& b);
        LuaValue shr(const LuaValue& a, const LuaValue& b);
        
        // Comparison operations with metamethod support
        bool eq(const LuaValue& a, const LuaValue& b);
        bool lt(const LuaValue& a, const LuaValue& b);
        bool le(const LuaValue& a, const LuaValue& b);
    private:
        LuaValue main_function_;
        std::vector<CallInfo> call_stack;
        std::vector<LuaValue> stack;
        int top;
        bool trace_execution = false;
    };

} // namespace luao