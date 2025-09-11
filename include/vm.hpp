#pragma once

#include <opcodes.hpp>
#include <closure.hpp>
#include <object.hpp>
#include <vector>
#include <memory>

#define CRITICAL_DUMP_CONTEXT_LINES 5

// metamethod key shortcuts
namespace mm {
    extern const luao::LuaValue __add       ;
    extern const luao::LuaValue __sub       ;
    extern const luao::LuaValue __mul       ;
    extern const luao::LuaValue __div       ;
    extern const luao::LuaValue __unm       ;
    extern const luao::LuaValue __mod       ;
    extern const luao::LuaValue __pow       ;
    extern const luao::LuaValue __idiv      ;
    extern const luao::LuaValue __band      ;
    extern const luao::LuaValue __bor       ;
    extern const luao::LuaValue __bxor      ;
    extern const luao::LuaValue __bnot      ;
    extern const luao::LuaValue __shl       ;
    extern const luao::LuaValue __shr       ;
    extern const luao::LuaValue __eq        ;
    extern const luao::LuaValue __lt        ;
    extern const luao::LuaValue __le        ;
    extern const luao::LuaValue __concat    ;
    extern const luao::LuaValue __len       ;
    extern const luao::LuaValue __tostring  ;
    extern const luao::LuaValue __metatable ;
    extern const luao::LuaValue __name      ;
    extern const luao::LuaValue __pairs     ;
    extern const luao::LuaValue __ipairs    ; /* deprecated */
    extern const luao::LuaValue __index     ;
    extern const luao::LuaValue __newindex  ;
    extern const luao::LuaValue __call      ;
    extern const luao::LuaValue __mode      ;
    extern const luao::LuaValue __close     ;
    extern const luao::LuaValue __gc        ;
    // luao extension (implement later, definition only)
    /* 
        # 
        # '__iterator' metamethod
        #
        returns are iterator function (used by for-each-do statements)
        the return value of the function returned by __iterator is arbitrary, and the return value is directly assigned to the argument of for.
        example:
        local arr = {1, 2, 3}
        local mt = {
            __iterator = function(self, t)
                local i = 0
                return function()
                    i = i + 1
                    return t[i]
                end
            end
        }
        setmetatable(arr, mt)
        -- for ... each value trying calls value's __iterator, __pairs, __ipairs metamethod.
        for v each arr do
            print(v)
        end
        
        outputs are '1' '2' '3'
    */
    extern const luao::LuaValue __iterator;
    /*
        #
        # '__newinstance' metamethod
        #
        This metamethod only applies to class.
        When a class is instantiated, the VM first calls the class's $init method and finally calls the __newinstance metamethod.

    */
    extern const luao::LuaValue __newinstance;
}

namespace luao {
    static LuaBool* TRUE_OBJ = new LuaBool(true);
    static LuaBool* FALSE_OBJ = new LuaBool(false);

    struct CallInfo;
    class VM;

    void dump_critical_error(const VM& vm, std::string err, CallInfo* frame, const Instruction* current_pc);

    struct CallInfo {
        std::shared_ptr<LuaClosure> closure;
        const Instruction* pc;
        int stack_base;

        CallInfo(std::shared_ptr<LuaClosure> closure, const Instruction* pc, int stack_base)
        : closure(std::move(closure)), pc(pc), stack_base(stack_base) {}
    };

    class VM {
    public:
        VM();
        void load(std::shared_ptr<LuaClosure> main_closure);
        void run();
        LuaValue get_stack_top();
        void set_top(int new_top);
        int get_top();
        const std::vector<LuaValue>& get_stack() const;
        std::vector<LuaValue>& get_stack_mutable();
        void set_trace(bool trace);
        bool as_bool(const LuaValue& value);
        const std::vector<CallInfo>& get_call_stack() const;
        std::vector<CallInfo>& get_call_stack_mutable();
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
        std::vector<CallInfo> call_stack;
        std::vector<LuaValue> stack;
        int top;
        bool trace_execution = false;
    };

} // namespace luao