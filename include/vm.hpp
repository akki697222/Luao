#pragma once

#include <opcodes.hpp>
#include <closure.hpp>
#include <object.hpp>
#include <vector>
#include <memory>
#include <list>

#define CRITICAL_DUMP_CONTEXT_LINES 5

// metamethod key shortcuts
namespace mm {
    extern const luao::LuaValue __add         ;
    extern const luao::LuaValue __sub         ;
    extern const luao::LuaValue __mul         ;
    extern const luao::LuaValue __div         ;
    extern const luao::LuaValue __unm         ;
    extern const luao::LuaValue __mod         ;
    extern const luao::LuaValue __pow         ;
    extern const luao::LuaValue __idiv        ;
    extern const luao::LuaValue __band        ;
    extern const luao::LuaValue __bor         ;
    extern const luao::LuaValue __bxor        ;
    extern const luao::LuaValue __bnot        ;
    extern const luao::LuaValue __shl         ;
    extern const luao::LuaValue __shr         ;
    extern const luao::LuaValue __eq          ;
    extern const luao::LuaValue __lt          ;
    extern const luao::LuaValue __le          ;
    extern const luao::LuaValue __concat      ;
    extern const luao::LuaValue __len         ;
    extern const luao::LuaValue __tostring    ;
    extern const luao::LuaValue __metatable   ;
    extern const luao::LuaValue __name        ;
    extern const luao::LuaValue __pairs       ;
    extern const luao::LuaValue __ipairs      ; /* deprecated */
    extern const luao::LuaValue __index       ;
    extern const luao::LuaValue __newindex    ;
    extern const luao::LuaValue __call        ;
    extern const luao::LuaValue __mode        ;
    extern const luao::LuaValue __close       ;
    extern const luao::LuaValue __gc          ;
    extern const luao::LuaValue __iterator    ;
    extern const luao::LuaValue __newinstance ;
}

namespace luao {
    static std::shared_ptr<LuaBool> TRUE_OBJ = std::make_shared<LuaBool>(true);
    static std::shared_ptr<LuaBool> FALSE_OBJ = std::make_shared<LuaBool>(false);

    struct CallInfo;
    class VM;
    class UpValue;
    class LuaValue;

    void dump_critical_error(VM& vm, std::string err);

    struct CallInfo {
        std::shared_ptr<LuaClosure> closure;
        const Instruction* pc;
        int stack_base;

        CallInfo(std::shared_ptr<LuaClosure> closure, const Instruction* pc, int stack_base)
        : closure(std::move(closure)), pc(pc), stack_base(stack_base) {}
    };

    struct LuaError : public std::exception {
        std::string message;
        LuaError(std::string msg) : message(std::move(msg)) {}
        const char* what() const noexcept override { return message.c_str(); }
    };

    class VM {
    public:
        VM();
        void load(std::shared_ptr<LuaClosure> main_closure);
        void run();
        LuaValue get_stack_top();
        void set_top(int new_top);
        int get_top();
        const std::vector<std::shared_ptr<LuaValue>>& get_stack() const;
        std::vector<std::shared_ptr<LuaValue>>& get_stack_mutable();
        void set_trace(bool trace);
        bool as_bool(const LuaValue& value);
        const std::vector<CallInfo>& get_call_stack() const;
        std::vector<CallInfo>& get_call_stack_mutable();
        const CallInfo& get_call_stack_top() const;
        const Instruction* get_current_pc();
        LuaValue get_upval_table(int upval_index, const LuaValue& key);
        
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
        
        // for debug
        Instruction* last_instruction;

        std::shared_ptr<UpValue> find_upvalue(int stack_index);
        std::list<std::shared_ptr<UpValue>> open_upvalues;
        void close_upvalues(int stack_index);

        friend class UpValue;
    private:
        std::vector<CallInfo> call_stack;
        std::vector<std::shared_ptr<LuaValue>> stack;
        int top;
        bool trace_execution = false;
    };

} // namespace luao