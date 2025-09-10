#include "vm.hpp"
#include "debug.hpp"
#include <table.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstring>
#include <opcodes.hpp>

// metamethod key shortcuts
namespace mm {
    static const luao::LuaValue __add       = luao::LuaValue(new luao::LuaString("__add"),       LuaType::STRING);
    static const luao::LuaValue __sub       = luao::LuaValue(new luao::LuaString("__sub"),       LuaType::STRING);
    static const luao::LuaValue __mul       = luao::LuaValue(new luao::LuaString("__mul"),       LuaType::STRING);
    static const luao::LuaValue __div       = luao::LuaValue(new luao::LuaString("__div"),       LuaType::STRING);
    static const luao::LuaValue __mod       = luao::LuaValue(new luao::LuaString("__mod"),       LuaType::STRING);
    static const luao::LuaValue __pow       = luao::LuaValue(new luao::LuaString("__pow"),       LuaType::STRING);
    static const luao::LuaValue __unm       = luao::LuaValue(new luao::LuaString("__unm"),       LuaType::STRING);
    static const luao::LuaValue __len       = luao::LuaValue(new luao::LuaString("__len"),       LuaType::STRING);
    static const luao::LuaValue __eq        = luao::LuaValue(new luao::LuaString("__eq"),        LuaType::STRING);
    static const luao::LuaValue __lt        = luao::LuaValue(new luao::LuaString("__lt"),        LuaType::STRING);
    static const luao::LuaValue __le        = luao::LuaValue(new luao::LuaString("__le"),        LuaType::STRING);
    static const luao::LuaValue __index     = luao::LuaValue(new luao::LuaString("__index"),     LuaType::STRING);
    static const luao::LuaValue __newindex  = luao::LuaValue(new luao::LuaString("__newindex"),  LuaType::STRING);
    static const luao::LuaValue __call      = luao::LuaValue(new luao::LuaString("__call"),      LuaType::STRING);
    static const luao::LuaValue __tostring  = luao::LuaValue(new luao::LuaString("__tostring"),  LuaType::STRING);
    static const luao::LuaValue __concat    = luao::LuaValue(new luao::LuaString("__concat"),    LuaType::STRING);
    static const luao::LuaValue __metatable = luao::LuaValue(new luao::LuaString("__metatable"), LuaType::STRING);
}

namespace luao {

#define GET_OPCODE(i)   (static_cast<OpCode>(((i) >> 0) & 0x7F))
#define GETARG_A(i)     (((i) >> 7) & 0xFF)
#define GETARG_sA(i)    (static_cast<int8_t>(GETARG_A(i)))
#define GETARG_B(i)     (((i) >> 16) & 0xFF)
#define GETARG_sB(i)    (static_cast<int8_t>(GETARG_B(i)))
#define GETARG_C(i)     (((i) >> 24) & 0xFF)
#define GETARG_sC(i)    (static_cast<int8_t>(GETARG_C(i)))
#define GETARG_Bx(i)    ((i) >> 15)
#define GETARG_sBx(i)   (static_cast<int>(GETARG_Bx(i)) - 65535)

void dump_critical_error(const VM& vm, std::string err, CallInfo* frame, const Instruction* current_pc) {
    std::cerr << "#\n";
    std::cerr << "# Luao VM\n";
    std::cerr << "#\n";
    std::cerr << "# VM Fatal Error: " << err << "\n";
    std::cerr << "#\n";
    std::cerr << "# Bytecode around PC\n";
    if (frame && frame->closure) {
        LuaFunction* func = frame->closure->getFunction();
        const auto& code = func->getBytecode();
        int idx = static_cast<int>(current_pc - &code[0]);
        int start = std::max(0, idx - CRITICAL_DUMP_CONTEXT_LINES);
        int end = std::min(static_cast<int>(code.size()), idx + CRITICAL_DUMP_CONTEXT_LINES + 1);
        for (int i = start; i < end; i++) {
            const Instruction& inst = code[i];
            std::cerr << (i == idx ? "# >" : "#  ")
                      << std::setw(3) << i << ": "
                      << std::setw(2) << static_cast<int>(GET_OPCODE(inst)) << "(" << to_string(GET_OPCODE(inst)) << ") "
                      << std::setw(3) << GETARG_A(inst) << " "
                      << std::setw(3) << GETARG_B(inst) << " "
                      << std::setw(3) << GETARG_C(inst) << "\n";
        }
    }
    std::cerr << "#\n";
    std::cerr << "# Stack\n";
    const auto& stack = vm.get_stack();
    int top = std::min(static_cast<int>(stack.size()), frame ? frame->stack_base + CRITICAL_DUMP_CONTEXT_LINES * 2 : static_cast<int>(stack.size()));
    for (int i = 0; i < top; i++) {
        const LuaValue& val = stack[i];
        std::cerr << "#" << std::setw(3) << i << ": " << val.typeName() << " ";
        if (val.getObject()) {
            std::cerr << val.getObject()->toString() << " (" << val.typeName() << ")";
        }
        std::cerr << "\n";
    }
    std::cerr << "#\n";
    std::cerr << "# Call Stack\n";
    if (!vm.get_stack().empty()) {
        const auto& call_stack = vm.get_call_stack();
        for (size_t i = 0; i < call_stack.size(); i++) {
            const CallInfo& f = call_stack[i];
            std::cerr << "#" << std::setw(3) << i << ":"
                      << " closure: " << (f.closure ? f.closure->getFunction()->typeName() : "null")
                      << " (" << f.closure << ")"
                      << ", stack_base=" << f.stack_base
                      << ", pc offset=" 
                      << (f.pc - &f.closure->getFunction()->getBytecode()[0])
                      << "\n";
        }
    }
    std::cerr << "#\n";
}

VM::VM() : top(0) {
    stack.resize(256);
}

void VM::load(LuaClosure* main_closure) {
    main_function_ = LuaValue(main_closure, LuaType::FUNCTION);
    call_stack.clear();
    stack.clear();
    stack.resize(256);
    top = 0;

    // Initialize global _ENV at R0
    stack[0] = LuaValue(new LuaTable(), LuaType::TABLE);
    // Initialize upvalues for main closure according to its prototype
    if (main_closure && main_closure->getFunction()) {
        auto* func = main_closure->getFunction();
        auto& updescs = func->getUpvalDescs();
        auto& upvals = main_closure->getUpvalues();
        upvals.reserve(updescs.size());
        for (const auto& desc : updescs) {
            UpValue* uv = nullptr;
            if (desc.inStack) {
                uv = new UpValue(&stack[desc.idx]);
            } else {
                // for main, capture from nowhere; fallback to R0
                uv = new UpValue(&stack[0]);
            }
            uv->retain();
            upvals.push_back(uv);
        }
    }

    call_stack.emplace_back(main_closure, &main_closure->getFunction()->getBytecode()[0], 0);
}

LuaValue VM::get_stack_top() {
    if (top > 0) {
        return stack[top - 1];
    }
    return LuaValue(); // Return nil if stack is empty
}

const std::vector<LuaValue>& VM::get_stack() const {
    return stack;
}

std::vector<LuaValue>& VM::get_stack_mutable() {
    return stack;
}

const CallInfo& VM::get_call_stack_top() const {
    return call_stack.back();
}

const std::vector<CallInfo>& VM::get_call_stack() const {
    return call_stack;
}

const Instruction* VM::get_current_pc() {
    return call_stack.back().pc;
}

void VM::set_trace(bool trace) {
    trace_execution = trace;
}

static luaNumber get_number_from_value(const LuaValue& val) {
    if (val.getType() == LuaType::NUMBER) {
        if (auto* num = dynamic_cast<const LuaNumber*>(val.getObject())) {
            return num->getValue();
        }
        if (auto* integer = dynamic_cast<const LuaInteger*>(val.getObject())) {
            return static_cast<luaNumber>(integer->getValue());
        }
    }
    return 0.0;
}

static const LuaValue& mm_key_from_C(int c) {
    switch (c) {
        case 0: return mm::__add;
        case 1: return mm::__sub;
        case 2: return mm::__mul;
        case 3: return mm::__div;
        case 4: return mm::__mod;
        case 5: return mm::__pow;
        case 6: return mm::__unm;
        case 7: return mm::__len;
        case 8: return mm::__eq;
        case 9: return mm::__lt;
        case 10: return mm::__le;
        case 11: return mm::__concat;
        default: return mm::__add;
    }
}

static bool try_call_c_metamethod(VM& vm, CallInfo* frame, int dest_a, const LuaValue& fn, const LuaValue& arg1, const LuaValue& arg2) {
    if (auto* cfunc = dynamic_cast<LuaNativeFunction*>(fn.getObject())) {
        int base_reg = frame->stack_base + dest_a;
        auto& st = vm.get_stack_mutable();
        st[base_reg + 0] = arg1;
        st[base_reg + 1] = arg2;
        int nret = cfunc->call(vm, base_reg, 2);
        if (nret > 0) {
            st[frame->stack_base + dest_a] = st[base_reg];
        }
        return nret > 0;
    }
    return false;
}

bool VM::as_bool(const LuaValue& value) {
    if (value.getType() == LuaType::NIL) {
        return false;
    }
    if (value.getType() == LuaType::BOOLEAN) {
        if (auto* b = dynamic_cast<const LuaBool*>(value.getObject())) {
            return b->getValue();
        }
    }
    return true;
}

// Arithmetic operations with metamethod support
LuaValue VM::add(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::NUMBER && b.getType() == LuaType::NUMBER) {
        auto* int_a = dynamic_cast<LuaInteger*>(a.getObject());
        auto* int_b = dynamic_cast<LuaInteger*>(b.getObject());
        
        if (int_a && int_b) {
            luaInt val_a = int_a->getValue();
            luaInt val_b = int_b->getValue();
            return LuaValue(new LuaInteger(val_a + val_b), LuaType::NUMBER);
        } else {
            luaNumber na = get_number_from_value(a);
            luaNumber nb = get_number_from_value(b);
            return LuaValue(new LuaNumber(na + nb), LuaType::NUMBER);
        }
    } else {
        // Metamethod handling would go here
        return LuaValue(); // nil for now
    }
}

LuaValue VM::sub(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::NUMBER && b.getType() == LuaType::NUMBER) {
        auto* int_a = dynamic_cast<LuaInteger*>(a.getObject());
        auto* int_b = dynamic_cast<LuaInteger*>(b.getObject());
        
        if (int_a && int_b) {
            luaInt val_a = int_a->getValue();
            luaInt val_b = int_b->getValue();
            return LuaValue(new LuaInteger(val_a - val_b), LuaType::NUMBER);
        } else {
            luaNumber na = get_number_from_value(a);
            luaNumber nb = get_number_from_value(b);
            return LuaValue(new LuaNumber(na - nb), LuaType::NUMBER);
        }
    } else {
        // Metamethod handling would go here
        return LuaValue(); // nil for now
    }
}

LuaValue VM::mul(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::NUMBER && b.getType() == LuaType::NUMBER) {
        auto* int_a = dynamic_cast<LuaInteger*>(a.getObject());
        auto* int_b = dynamic_cast<LuaInteger*>(b.getObject());
        
        if (int_a && int_b) {
            luaInt val_a = int_a->getValue();
            luaInt val_b = int_b->getValue();
            return LuaValue(new LuaInteger(val_a * val_b), LuaType::NUMBER);
        } else {
            luaNumber na = get_number_from_value(a);
            luaNumber nb = get_number_from_value(b);
            return LuaValue(new LuaNumber(na * nb), LuaType::NUMBER);
        }
    } else {
        // Metamethod handling would go here
        return LuaValue(); // nil for now
    }
}

LuaValue VM::div(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::NUMBER && b.getType() == LuaType::NUMBER) {
        luaNumber na = get_number_from_value(a);
        luaNumber nb = get_number_from_value(b);
        return LuaValue(new LuaNumber(na / nb), LuaType::NUMBER);
    } else {
        // Metamethod handling would go here
        return LuaValue(); // nil for now
    }
}

LuaValue VM::mod(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::NUMBER && b.getType() == LuaType::NUMBER) {
        luaNumber na = get_number_from_value(a);
        luaNumber nb = get_number_from_value(b);
        luaNumber res = std::fmod(na, nb);
        if (res < 0) res += nb;
        return LuaValue(new LuaNumber(res), LuaType::NUMBER);
    } else {
        // Metamethod handling would go here
        return LuaValue(); // nil for now
    }
}

LuaValue VM::pow(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::NUMBER && b.getType() == LuaType::NUMBER) {
        luaNumber na = get_number_from_value(a);
        luaNumber nb = get_number_from_value(b);
        return LuaValue(new LuaNumber(std::pow(na, nb)), LuaType::NUMBER);
    } else {
        // Metamethod handling would go here
        return LuaValue(); // nil for now
    }
}

LuaValue VM::idiv(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::NUMBER && b.getType() == LuaType::NUMBER) {
        luaNumber na = get_number_from_value(a);
        luaNumber nb = get_number_from_value(b);
        return LuaValue(new LuaNumber(std::floor(na / nb)), LuaType::NUMBER);
    } else {
        // Metamethod handling would go here
        return LuaValue(); // nil for now
    }
}

LuaValue VM::unm(const LuaValue& a) {
    if (a.getType() == LuaType::NUMBER) {
        if (auto* int_a = dynamic_cast<LuaInteger*>(a.getObject())) {
            return LuaValue(new LuaInteger(-int_a->getValue()), LuaType::NUMBER);
        } else if (auto* num_a = dynamic_cast<LuaNumber*>(a.getObject())) {
            return LuaValue(new LuaNumber(-num_a->getValue()), LuaType::NUMBER);
        }
    } else {
        // Metamethod handling would go here
    }
    return LuaValue(); // nil for now
}

LuaValue VM::len(const LuaValue& a) {
    if (a.getType() == LuaType::STRING) {
        auto* str = dynamic_cast<LuaString*>(a.getObject());
        return LuaValue(new LuaInteger(str->getValue().size()), LuaType::NUMBER);
    } else if (a.getType() == LuaType::TABLE) {
        auto* table = dynamic_cast<LuaTable*>(a.getObject());
        LuaValue mm = table->getMetamethod(mm::__len);
        if (mm.getObject()) {
            // metamethod handling would go here
            return LuaValue(); // nil for now
        } else {
            return table->vlen();
        }
    } else {
        if (auto* gc = dynamic_cast<LuaGCObject*>(a.getObject())) {
            LuaValue mm = gc->getMetamethod(mm::__len);
            if (mm.getObject()) {
                // metamethod handling would go here
                return LuaValue(); // nil for now
            }
        }
    }
    return LuaValue(); // nil for now
}

LuaValue VM::concat(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::STRING && b.getType() == LuaType::STRING) {
        auto* str_a = dynamic_cast<LuaString*>(a.getObject());
        auto* str_b = dynamic_cast<LuaString*>(b.getObject());
        return LuaValue(new LuaString(str_a->getValue() + str_b->getValue()), LuaType::STRING);
    } else {
        // Metamethod handling would go here
        return LuaValue(); // nil for now
    }
}

// Bitwise operations with metamethod support
LuaValue VM::band(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::NUMBER && b.getType() == LuaType::NUMBER) {
        luaInt ia = static_cast<luaInt>(get_number_from_value(a));
        luaInt ib = static_cast<luaInt>(get_number_from_value(b));
        return LuaValue(new LuaInteger(ia & ib), LuaType::NUMBER);
    } else {
        // Metamethod handling would go here
        return LuaValue(); // nil for now
    }
}

LuaValue VM::bor(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::NUMBER && b.getType() == LuaType::NUMBER) {
        luaInt ia = static_cast<luaInt>(get_number_from_value(a));
        luaInt ib = static_cast<luaInt>(get_number_from_value(b));
        return LuaValue(new LuaInteger(ia | ib), LuaType::NUMBER);
    } else {
        // Metamethod handling would go here
        return LuaValue(); // nil for now
    }
}

LuaValue VM::bxor(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::NUMBER && b.getType() == LuaType::NUMBER) {
        luaInt ia = static_cast<luaInt>(get_number_from_value(a));
        luaInt ib = static_cast<luaInt>(get_number_from_value(b));
        return LuaValue(new LuaInteger(ia ^ ib), LuaType::NUMBER);
    } else {
        // Metamethod handling would go here
        return LuaValue(); // nil for now
    }
}

LuaValue VM::bnot(const LuaValue& a) {
    if (a.getType() == LuaType::NUMBER) {
        luaInt ia = static_cast<luaInt>(get_number_from_value(a));
        return LuaValue(new LuaInteger(~ia), LuaType::NUMBER);
    } else {
        // Metamethod handling would go here
        return LuaValue(); // nil for now
    }
}

LuaValue VM::shl(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::NUMBER && b.getType() == LuaType::NUMBER) {
        luaInt ia = static_cast<luaInt>(get_number_from_value(a));
        luaInt ib = static_cast<luaInt>(get_number_from_value(b));
        return LuaValue(new LuaInteger(ia << ib), LuaType::NUMBER);
    } else {
        // Metamethod handling would go here
        return LuaValue(); // nil for now
    }
}

LuaValue VM::shr(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::NUMBER && b.getType() == LuaType::NUMBER) {
        luaInt ia = static_cast<luaInt>(get_number_from_value(a));
        luaInt ib = static_cast<luaInt>(get_number_from_value(b));
        return LuaValue(new LuaInteger(ia >> ib), LuaType::NUMBER);
    } else {
        // Metamethod handling would go here
        return LuaValue(); // nil for now
    }
}

// Comparison operations with metamethod support
bool VM::eq(const LuaValue& a, const LuaValue& b) {
    if (a.getType() != b.getType()) {
        return false;
    }
    
    if (a.getType() == LuaType::NIL) {
        return true;
    }
    
    if (a.getType() == LuaType::NUMBER) {
        luaNumber na = get_number_from_value(a);
        luaNumber nb = get_number_from_value(b);
        return na == nb;
    }
    
    if (a.getType() == LuaType::STRING) {
        auto* str_a = dynamic_cast<LuaString*>(a.getObject());
        auto* str_b = dynamic_cast<LuaString*>(b.getObject());
        return str_a->getValue() == str_b->getValue();
    }
    
    if (a.getType() == LuaType::BOOLEAN) {
        auto* bool_a = dynamic_cast<LuaBool*>(a.getObject());
        auto* bool_b = dynamic_cast<LuaBool*>(b.getObject());
        return bool_a->getValue() == bool_b->getValue();
    }
    
    // For other types, compare object pointers
    return a.getObject() == b.getObject();
}

bool VM::lt(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::NUMBER && b.getType() == LuaType::NUMBER) {
        luaNumber na = get_number_from_value(a);
        luaNumber nb = get_number_from_value(b);
        return na < nb;
    } else if (a.getType() == LuaType::STRING && b.getType() == LuaType::STRING) {
        auto* str_a = dynamic_cast<LuaString*>(a.getObject());
        auto* str_b = dynamic_cast<LuaString*>(b.getObject());
        return str_a->getValue() < str_b->getValue();
    } else {
        // Metamethod handling would go here
        return false;
    }
}

bool VM::le(const LuaValue& a, const LuaValue& b) {
    if (a.getType() == LuaType::NUMBER && b.getType() == LuaType::NUMBER) {
        luaNumber na = get_number_from_value(a);
        luaNumber nb = get_number_from_value(b);
        return na <= nb;
    } else if (a.getType() == LuaType::STRING && b.getType() == LuaType::STRING) {
        auto* str_a = dynamic_cast<LuaString*>(a.getObject());
        auto* str_b = dynamic_cast<LuaString*>(b.getObject());
        return str_a->getValue() <= str_b->getValue();
    } else {
        // Metamethod handling would go here
        return false;
    }
}

void VM::run() {
    while (!call_stack.empty()) {
        CallInfo* frame = &call_stack.back();
        const Instruction* pc = frame->pc;
        LuaFunction* func = frame->closure->getFunction();

        for (;;) {
            Instruction i = *pc++;
            if (trace_execution) {
                std::cout << disassemble_instruction(i, func) << std::endl;
            }
            OpCode op = GET_OPCODE(i);

            switch (op) {
                case OpCode::MOVE: {
                    int a = GETARG_A(i); /* args are 'A B' */
                    int b = GETARG_B(i);
                    stack[frame->stack_base + a] = stack[frame->stack_base + b];
                    break;
                }
                case OpCode::LOADI: {
                    int a = GETARG_A(i); /* args are 'A sBx' */
                    int sbx = GETARG_sBx(i);
                    stack[frame->stack_base + a] = LuaValue(new LuaInteger(sbx), LuaType::NUMBER);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::LOADF: {
                    int a = GETARG_A(i); /* args are 'A sBx' */
                    int sbx = GETARG_sBx(i);
                    stack[frame->stack_base + a] = LuaValue(new LuaNumber(static_cast<luaNumber>(sbx)), LuaType::NUMBER);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::LOADK: {
                    int a = GETARG_A(i); /* args are 'A Bx' */
                    int bx = GETARG_Bx(i);
                    stack[frame->stack_base + a] = func->getConstants()[bx];
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::LOADKX: {
                    int a = GETARG_A(i);
                    // LOADKX uses the next instruction as extra argument
                    Instruction extra = *pc++;
                    int bx = GETARG_Bx(extra);
                    stack[frame->stack_base + a] = func->getConstants()[bx];
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::LOADFALSE: {
                    int a = GETARG_A(i); /* args are 'A' */
                    stack[frame->stack_base + a] = LuaValue(FALSE_OBJ, LuaType::BOOLEAN);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::LFALSESKIP: {
                    int a = GETARG_A(i); /* args are 'A' */
                    stack[frame->stack_base + a] = LuaValue(FALSE_OBJ, LuaType::BOOLEAN);
                    top = frame->stack_base + a + 1;
                    pc++; // skip next instruction
                    break;
                }
                case OpCode::LOADTRUE: {
                    int a = GETARG_A(i); /* args are 'A' */
                    stack[frame->stack_base + a] = LuaValue(TRUE_OBJ, LuaType::BOOLEAN);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::LOADNIL: {
                    int a = GETARG_A(i); /* args are 'A B' */
                    int b = GETARG_B(i);
                    for (int j = 0; j <= b; j++) {
                        stack[frame->stack_base + a + j] = LuaValue(); // nil
                    }
                    top = frame->stack_base + a + b + 1;
                    break;
                }
                // case OpCode::GETUPVAL: {}
                // case OpCode::SETUPVAL: {}
                case OpCode::GETTABUP: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    auto& upvals = frame->closure->getUpvalues();
                    if (b < 0 || b >= static_cast<int>(upvals.size())) {
                        throw std::runtime_error("GETTABUP: invalid upvalue index");
                    }
                    LuaValue t = upvals[b]->getValue();
                    LuaValue k = func->getConstants()[c];
                    if (auto* table = dynamic_cast<LuaTable*>(t.getObject())) {
                        stack[frame->stack_base + a] = table->get(k);
                    } else {
                        std::cerr << "GETTABUP on non-table upvalue" << std::endl;
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::GETTABLE: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);

                    LuaValue t = stack[frame->stack_base + b];
                    LuaValue k = stack[frame->stack_base + c];

                    if (t.getType() == LuaType::TABLE) {
                        if (auto* table = dynamic_cast<LuaTable*>(t.getObject())) {
                            stack[frame->stack_base + a] = table->get(k);
                        } else {
                            // metamethod
                        }
                    } else {
                        if (auto* gc = dynamic_cast<LuaGCObject*>(t.getObject())) {
                            // metamethod
                        } else {
                            std::cerr << "Attempt to index a " << t.getObject()->typeName() << " value" << std::endl;
                        }
                    }
                    break;
                }
                case OpCode::GETI: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);

                    LuaValue t = stack[frame->stack_base + b];

                    if (t.getType() == LuaType::TABLE) {
                        if (auto* table = dynamic_cast<LuaTable*>(t.getObject())) {
                            stack[frame->stack_base + a] = table->get(c);
                        } else {
                            // metamethod
                        }
                    } else {
                        if (auto* gc = dynamic_cast<LuaGCObject*>(t.getObject())) {
                            // metamethod
                        } else {
                            std::cerr << "Attempt to index a " << t.getObject()->typeName() << " value" << std::endl;
                        }
                    }

                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::GETFIELD: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);

                    LuaValue t = stack[frame->stack_base + b];
                    LuaValue k = func->getConstants()[c];

                    if (t.getType() == LuaType::TABLE) {
                        if (auto* table = dynamic_cast<LuaTable*>(t.getObject())) {
                            stack[frame->stack_base + a] = table->get(k);
                        } else {
                            // metamethod
                        }
                    } else {
                        if (auto* gc = dynamic_cast<LuaGCObject*>(t.getObject())) {
                            // metamethod
                        } else {
                            std::cerr << "Attempt to index a " << t.getObject()->typeName() << " value" << std::endl;
                        }
                    }
                    break;
                }
                case OpCode::SETTABUP: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    auto& upvals = frame->closure->getUpvalues();
                    if (a < 0 || a >= static_cast<int>(upvals.size())) {
                        throw std::runtime_error("SETTABUP: invalid upvalue index");
                    }
                    LuaValue t = upvals[a]->getValue();
                    LuaValue k = func->getConstants()[b];
                    LuaValue v = stack[frame->stack_base + c];
                    if (auto* table = dynamic_cast<LuaTable*>(t.getObject())) {
                        table->set(k, v);
                    } else {
                        std::cerr << "SETTABUP on non-table upvalue" << std::endl;
                    }
                    break;
                }
                case OpCode::SETTABLE: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);

                    LuaValue t = stack[frame->stack_base + a];
                    LuaValue k = stack[frame->stack_base + b];
                    LuaValue v = stack[frame->stack_base + c];

                    if (t.getType() == LuaType::TABLE) {
                        if (auto* table = dynamic_cast<LuaTable*>(t.getObject())) {
                            table->set(k, v);
                        } else {
                            // metamethod
                        }
                    } else {
                        if (auto* gc = dynamic_cast<LuaGCObject*>(t.getObject())) {
                            // metamethod
                        } else {
                            std::cerr << "Attempt to index a " << t.getObject()->typeName() << " value" << std::endl;
                        }
                    }

                    break;
                }
                case OpCode::SETI: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);

                    LuaValue t = stack[frame->stack_base + a];
                    LuaValue v = stack[frame->stack_base + c];

                    if (t.getType() == LuaType::TABLE) {
                        if (auto* table = dynamic_cast<LuaTable*>(t.getObject())) {
                            table->set(b, v);
                        } else {
                            // metamethod
                        }
                    } else {
                        if (auto* gc = dynamic_cast<LuaGCObject*>(t.getObject())) {
                            // metamethod
                        } else {
                            std::cerr << "Attempt to index a " << t.getObject()->typeName() << " value" << std::endl;
                        }
                    }

                    break;
                }
                case OpCode::SETFIELD: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);

                    LuaValue t = stack[frame->stack_base + a];
                    LuaValue k = func->getConstants()[b];
                    LuaValue v = stack[frame->stack_base + c];

                    if (t.getType() == LuaType::TABLE) {
                        if (auto* table = dynamic_cast<LuaTable*>(t.getObject())) {
                            table->set(k, v);
                        } else {
                            // metamethod
                        }
                    } else {
                        if (auto* gc = dynamic_cast<LuaGCObject*>(t.getObject())) {
                            // metamethod
                        } else {
                            std::cerr << "Attempt to index a " << t.getObject()->typeName() << " value" << std::endl;
                        }
                    }

                    break;
                }
                case OpCode::NEWTABLE: {
                    int a = GETARG_A(i); /* args are 'A B C k' */
                    /* B, C, k are unused for now */
                    stack[frame->stack_base + a] = LuaValue(new LuaTable(), LuaType::TABLE);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::SELF: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    
                    LuaValue self = stack[frame->stack_base + b];
                    LuaValue method_key = stack[frame->stack_base + c];
                    
                    // R[A+1] := R[B] (self)
                    stack[frame->stack_base + a + 1] = self;
                    
                    // R[A] := R[B][RK(C):string] (method)
                    if (self.getType() == LuaType::TABLE) {
                        if (auto* table = dynamic_cast<LuaTable*>(self.getObject())) {
                            stack[frame->stack_base + a] = table->get(method_key);
                        } else {
                            // metamethod handling would go here
                        }
                    } else {
                        if (auto* gc = dynamic_cast<LuaGCObject*>(self.getObject())) {
                            // metamethod handling would go here
                        } else {
                            std::cerr << "Attempt to index a " << self.typeName() << " value" << std::endl;
                        }
                    }
                    top = frame->stack_base + a + 2;
                    break;
                }
                case OpCode::ADDI: {
                    int a = GETARG_A(i); /* args are 'A B sC' */
                    int b = GETARG_B(i);
                    int sc = GETARG_sC(i);
                    LuaValue rb = stack[frame->stack_base + b];

                    if (rb.getType() == LuaType::NUMBER) {
                        if (auto* int_b = dynamic_cast<LuaInteger*>(rb.getObject())) {
                            luaInt val_b = int_b->getValue();
                            luaInt res = val_b + sc;
                            stack[frame->stack_base + a] = LuaValue(new LuaInteger(res), LuaType::NUMBER);
                        } else if (auto* num_b = dynamic_cast<LuaNumber*>(rb.getObject())) {
                            luaNumber nb = num_b->getValue();
                            luaNumber res = nb + static_cast<luaNumber>(sc);
                            stack[frame->stack_base + a] = LuaValue(new LuaNumber(res), LuaType::NUMBER);
                        }
                    } else {
                        // metamethod fallback for ADDI: R[A] = R[B] + sC
                        LuaValue imm(new LuaInteger(sc), LuaType::NUMBER);
                        auto try_mm = [&](const LuaValue& v1, const LuaValue& v2) -> bool {
                            if (auto* gc = dynamic_cast<LuaGCObject*>(v1.getObject())) {
                                LuaValue mmf = gc->getMetamethod(mm::__add);
                                if (mmf.getType() == LuaType::FUNCTION) {
                                    if (try_call_c_metamethod(*this, frame, a, mmf, v1, v2)) { top = frame->stack_base + a + 1; return true; }
                                }
                            }
                            return false;
                        };
                        if (try_mm(rb, imm) || try_mm(imm, rb)) { break; }
                        std::cerr << "attempt to perform ADDI via metamethod on unsupported types" << std::endl;
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::ADDK: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue kc = func->getConstants()[c];
                    
                    if (rb.getType() == LuaType::NUMBER && kc.getType() == LuaType::NUMBER) {
                        luaNumber nb = get_number_from_value(rb);
                        luaNumber nc = get_number_from_value(kc);
                        stack[frame->stack_base + a] = LuaValue(new LuaNumber(nb + nc), LuaType::NUMBER);
                    } else {
                        // metamethod handling would go here
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::SUBK: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue kc = func->getConstants()[c];
                    
                    if (rb.getType() == LuaType::NUMBER && kc.getType() == LuaType::NUMBER) {
                        luaNumber nb = get_number_from_value(rb);
                        luaNumber nc = get_number_from_value(kc);
                        stack[frame->stack_base + a] = LuaValue(new LuaNumber(nb - nc), LuaType::NUMBER);
                    } else {
                        // metamethod handling would go here
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::MULK: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue kc = func->getConstants()[c];
                    
                    if (rb.getType() == LuaType::NUMBER && kc.getType() == LuaType::NUMBER) {
                        luaNumber nb = get_number_from_value(rb);
                        luaNumber nc = get_number_from_value(kc);
                        stack[frame->stack_base + a] = LuaValue(new LuaNumber(nb * nc), LuaType::NUMBER);
                    } else {
                        // metamethod handling would go here
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::MODK: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue kc = func->getConstants()[c];
                    
                    if (rb.getType() == LuaType::NUMBER && kc.getType() == LuaType::NUMBER) {
                        luaNumber nb = get_number_from_value(rb);
                        luaNumber nc = get_number_from_value(kc);
                        luaNumber res = std::fmod(nb, nc);
                        if (res < 0) res += nc;
                        stack[frame->stack_base + a] = LuaValue(new LuaNumber(res), LuaType::NUMBER);
                    } else {
                        // metamethod handling would go here
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::POWK: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue kc = func->getConstants()[c];
                    
                    if (rb.getType() == LuaType::NUMBER && kc.getType() == LuaType::NUMBER) {
                        luaNumber nb = get_number_from_value(rb);
                        luaNumber nc = get_number_from_value(kc);
                        stack[frame->stack_base + a] = LuaValue(new LuaNumber(std::pow(nb, nc)), LuaType::NUMBER);
                    } else {
                        // metamethod handling would go here
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::DIVK: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue kc = func->getConstants()[c];
                    
                    if (rb.getType() == LuaType::NUMBER && kc.getType() == LuaType::NUMBER) {
                        luaNumber nb = get_number_from_value(rb);
                        luaNumber nc = get_number_from_value(kc);
                        stack[frame->stack_base + a] = LuaValue(new LuaNumber(nb / nc), LuaType::NUMBER);
                    } else {
                        // metamethod handling would go here
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::IDIVK: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue kc = func->getConstants()[c];
                    
                    if (rb.getType() == LuaType::NUMBER && kc.getType() == LuaType::NUMBER) {
                        luaNumber nb = get_number_from_value(rb);
                        luaNumber nc = get_number_from_value(kc);
                        luaNumber res = std::floor(nb / nc);
                        stack[frame->stack_base + a] = LuaValue(new LuaNumber(res), LuaType::NUMBER);
                    } else {
                        // metamethod handling would go here
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::ADD: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue rc = stack[frame->stack_base + c];
                    stack[frame->stack_base + a] = add(rb, rc);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::SUB: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue rc = stack[frame->stack_base + c];
                    stack[frame->stack_base + a] = sub(rb, rc);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::MUL: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue rc = stack[frame->stack_base + c];
                    stack[frame->stack_base + a] = mul(rb, rc);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::DIV: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue rc = stack[frame->stack_base + c];
                    stack[frame->stack_base + a] = div(rb, rc);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::MOD: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue rc = stack[frame->stack_base + c];
                    stack[frame->stack_base + a] = mod(rb, rc);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::POW: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue rc = stack[frame->stack_base + c];
                    stack[frame->stack_base + a] = pow(rb, rc);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::IDIV: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue rc = stack[frame->stack_base + c];
                    stack[frame->stack_base + a] = idiv(rb, rc);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::BAND: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue rc = stack[frame->stack_base + c];
                    stack[frame->stack_base + a] = band(rb, rc);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::BOR: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue rc = stack[frame->stack_base + c];
                    stack[frame->stack_base + a] = bor(rb, rc);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::BXOR: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue rc = stack[frame->stack_base + c];
                    stack[frame->stack_base + a] = bxor(rb, rc);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::SHL: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue rc = stack[frame->stack_base + c];
                    stack[frame->stack_base + a] = shl(rb, rc);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::SHR: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue rc = stack[frame->stack_base + c];
                    stack[frame->stack_base + a] = shr(rb, rc);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::BANDK: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue kc = func->getConstants()[c];
                    
                    if (rb.getType() == LuaType::NUMBER && kc.getType() == LuaType::NUMBER) {
                        luaInt ib = static_cast<luaInt>(get_number_from_value(rb));
                        luaInt ic = static_cast<luaInt>(get_number_from_value(kc));
                        stack[frame->stack_base + a] = LuaValue(new LuaInteger(ib & ic), LuaType::NUMBER);
                    } else {
                        // metamethod handling would go here
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::BORK: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue kc = func->getConstants()[c];
                    
                    if (rb.getType() == LuaType::NUMBER && kc.getType() == LuaType::NUMBER) {
                        luaInt ib = static_cast<luaInt>(get_number_from_value(rb));
                        luaInt ic = static_cast<luaInt>(get_number_from_value(kc));
                        stack[frame->stack_base + a] = LuaValue(new LuaInteger(ib | ic), LuaType::NUMBER);
                    } else {
                        // metamethod handling would go here
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::BXORK: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue kc = func->getConstants()[c];
                    
                    if (rb.getType() == LuaType::NUMBER && kc.getType() == LuaType::NUMBER) {
                        luaInt ib = static_cast<luaInt>(get_number_from_value(rb));
                        luaInt ic = static_cast<luaInt>(get_number_from_value(kc));
                        stack[frame->stack_base + a] = LuaValue(new LuaInteger(ib ^ ic), LuaType::NUMBER);
                    } else {
                        // metamethod handling would go here
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::SHRI: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int sc = GETARG_sC(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    
                    if (rb.getType() == LuaType::NUMBER) {
                        luaInt ib = static_cast<luaInt>(get_number_from_value(rb));
                        luaInt res = ib >> sc;
                        stack[frame->stack_base + a] = LuaValue(new LuaInteger(res), LuaType::NUMBER);
                    } else {
                        // metamethod handling would go here
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::SHLI: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int sc = GETARG_sC(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    
                    if (rb.getType() == LuaType::NUMBER) {
                        luaInt ib = static_cast<luaInt>(get_number_from_value(rb));
                        luaInt res = ib << sc;
                        stack[frame->stack_base + a] = LuaValue(new LuaInteger(res), LuaType::NUMBER);
                    } else {
                        // metamethod handling would go here
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::UNM: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    stack[frame->stack_base + a] = unm(rb);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::BNOT: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    stack[frame->stack_base + a] = bnot(rb);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::NOT: {
                    int a = GETARG_A(i); /* args are 'A B' */
                    int b = GETARG_B(i);
                    stack[frame->stack_base + a] = LuaValue(as_bool(stack[frame->stack_base + b]) ? FALSE_OBJ : TRUE_OBJ, LuaType::BOOLEAN);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::LEN: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    stack[frame->stack_base + a] = len(rb);
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::CONCAT: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    if (b > c) {
                        stack[frame->stack_base + a] = LuaValue(new LuaString(""), LuaType::STRING);
                    } else {
                        LuaValue result = stack[frame->stack_base + b];
                        for (int j = b + 1; j <= c; j++) {
                            result = concat(result, stack[frame->stack_base + j]);
                        }
                        stack[frame->stack_base + a] = result;
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::CLOSE: {
                    int a = GETARG_A(i);
                    // Close all upvalues >= R[A]
                    // This is a simplified implementation
                    // In a full implementation, we would need to track which upvalues are open
                    // and close them when they go out of scope
                    break;
                }
                case OpCode::TBC: {
                    int a = GETARG_A(i);
                    // Mark variable A "to be closed"
                    // This is a simplified implementation
                    // In a full implementation, we would mark the variable for closure
                    // when it goes out of scope
                    break;
                }
                case OpCode::EQ: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int k = GETARG_C(i);
                    LuaValue ra = stack[frame->stack_base + a];
                    LuaValue rb = stack[frame->stack_base + b];
                    
                    bool result = eq(ra, rb);
                    if (k == 0) result = !result;
                    if (!result) pc++;
                    break;
                }
                case OpCode::LT: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int k = GETARG_C(i);
                    LuaValue ra = stack[frame->stack_base + a];
                    LuaValue rb = stack[frame->stack_base + b];
                    
                    bool result = lt(ra, rb);
                    if (k == 0) result = !result;
                    if (!result) pc++;
                    break;
                }
                case OpCode::LE: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int k = GETARG_C(i);
                    LuaValue ra = stack[frame->stack_base + a];
                    LuaValue rb = stack[frame->stack_base + b];
                    
                    bool result = le(ra, rb);
                    if (k == 0) result = !result;
                    if (!result) pc++;
                    break;
                }
                case OpCode::EQK: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int k = GETARG_C(i);
                    LuaValue ra = stack[frame->stack_base + a];
                    LuaValue kb = func->getConstants()[b];
                    
                    bool result = eq(ra, kb);
                    if (k == 0) result = !result;
                    if (!result) pc++;
                    break;
                }
                case OpCode::EQI: {
                    int a = GETARG_A(i);
                    int sb = GETARG_sB(i);
                    int k = GETARG_C(i);
                    LuaValue ra = stack[frame->stack_base + a];
                    LuaValue ib(new LuaInteger(sb), LuaType::NUMBER);
                    
                    bool result = eq(ra, ib);
                    if (k == 0) result = !result;
                    if (!result) pc++;
                    break;
                }
                case OpCode::LTI: {
                    int a = GETARG_A(i);
                    int sb = GETARG_sB(i);
                    int k = GETARG_C(i);
                    LuaValue ra = stack[frame->stack_base + a];
                    LuaValue ib(new LuaInteger(sb), LuaType::NUMBER);
                    
                    bool result = lt(ra, ib);
                    if (k == 0) result = !result;
                    if (!result) pc++;
                    break;
                }
                case OpCode::LEI: {
                    int a = GETARG_A(i);
                    int sb = GETARG_sB(i);
                    int k = GETARG_C(i);
                    LuaValue ra = stack[frame->stack_base + a];
                    LuaValue ib(new LuaInteger(sb), LuaType::NUMBER);
                    
                    bool result = le(ra, ib);
                    if (k == 0) result = !result;
                    if (!result) pc++;
                    break;
                }
                case OpCode::GTI: {
                    int a = GETARG_A(i);
                    int sb = GETARG_sB(i);
                    int k = GETARG_C(i);
                    LuaValue ra = stack[frame->stack_base + a];
                    LuaValue ib(new LuaInteger(sb), LuaType::NUMBER);
                    
                    bool result = lt(ib, ra); // a > b is equivalent to b < a
                    if (k == 0) result = !result;
                    if (!result) pc++;
                    break;
                }
                case OpCode::GEI: {
                    int a = GETARG_A(i);
                    int sb = GETARG_sB(i);
                    int k = GETARG_C(i);
                    LuaValue ra = stack[frame->stack_base + a];
                    LuaValue ib(new LuaInteger(sb), LuaType::NUMBER);
                    
                    bool result = le(ib, ra); // a >= b is equivalent to b <= a
                    if (k == 0) result = !result;
                    if (!result) pc++;
                    break;
                }
                case OpCode::TEST: {
                    int a = GETARG_A(i);
                    int k = GETARG_C(i);
                    LuaValue ra = stack[frame->stack_base + a];
                    
                    bool result = as_bool(ra);
                    if (k == 0) result = !result;
                    if (!result) pc++;
                    break;
                }
                case OpCode::TESTSET: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int k = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    
                    bool result = as_bool(rb);
                    if (k == 0) result = !result;
                    if (result) {
                        stack[frame->stack_base + a] = rb;
                    } else {
                        pc++;
                    }
                    break;
                }
                case OpCode::JMP: {
                    int sJ = GETARG_sA(i);
                    pc += sJ - 1;
                    break;
                }
                case OpCode::CALL: {
                    int a = GETARG_A(i);
                    LuaValue func_val = stack[frame->stack_base + a];
                    if (func_val.getType() == LuaType::FUNCTION) {
                        if (auto* closure = dynamic_cast<LuaClosure*>(func_val.getObject())) {
                            frame->pc = pc;
                            call_stack.emplace_back(closure, &closure->getFunction()->getBytecode()[0], frame->stack_base + a + 1);
                            break; // Continue to the new frame's execution loop
                        }
                        // native cfunction support
                        if (auto* cfunc = dynamic_cast<LuaNativeFunction*>(func_val.getObject())) {
                            // advance pc for current frame so we don't re-execute CALL
                            frame->pc = pc;
                            // For now, ignore B/C; treat as fixed arity 0 args, 1 return (provided by cfunc)
                            int num_args = 0; // TODO: respect encoded B later
                            int base_reg = frame->stack_base + a + 1;
                            int nret = cfunc->call(*this, base_reg, num_args);
                            // place results at R[A..]
                            for (int j = 0; j < nret; ++j) {
                                stack[frame->stack_base + a + j] = stack[base_reg + j];
                            }
                            top = frame->stack_base + a + nret;
                            break;
                        }
                    }
                    std::cerr << "Attempt to call a " << func_val.typeName() << " value" << std::endl;
                    return; // or error
                }
                case OpCode::TAILCALL: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue func_val = stack[frame->stack_base + a];
                    
                    if (func_val.getType() == LuaType::FUNCTION) {
                        if (auto* closure = dynamic_cast<LuaClosure*>(func_val.getObject())) {
                            // For tail call, we replace the current frame instead of adding a new one
                            frame->closure = closure;
                            frame->pc = &closure->getFunction()->getBytecode()[0];
                            frame->stack_base = frame->stack_base + a; // Adjust stack base
                            break;
                        }
                        // native cfunction support for tail call
                        if (auto* cfunc = dynamic_cast<LuaNativeFunction*>(func_val.getObject())) {
                            int num_args = b > 0 ? b - 1 : 0;
                            int base_reg = frame->stack_base + a + 1;
                            int nret = cfunc->call(*this, base_reg, num_args);
                            // For tail call, we return directly
                            call_stack.pop_back();
                            break;
                        }
                    }
                    std::cerr << "Attempt to tail call a " << func_val.typeName() << " value" << std::endl;
                    return; // or error
                }
                case OpCode::RETURN: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int n_results = b > 0 ? b - 1 : 0; // Simplified for now

                    if (call_stack.size() > 1) {
                        CallInfo& caller_frame = call_stack[call_stack.size() - 2];
                        Instruction call_i = *(caller_frame.pc - 1);
                        int result_reg = GETARG_A(call_i);
                        for (int j = 0; j < n_results; j++) {
                            stack[caller_frame.stack_base + result_reg + j] = stack[frame->stack_base + a + j];
                        }
                        top = caller_frame.stack_base + result_reg + n_results;
                    } else {
                        // Returning from top-level
                        for (int j = 0; j < n_results; j++) {
                            stack[j] = stack[frame->stack_base + a + j];
                        }
                        top = n_results;
                    }
                    call_stack.pop_back();
                    break;
                }
                case OpCode::RETURN0: {
                    if (call_stack.size() > 1) {
                        CallInfo& caller_frame = call_stack[call_stack.size() - 2];
                        top = caller_frame.stack_base + GETARG_A(*(caller_frame.pc - 1));
                    } else {
                        top = 0;
                    }
                    call_stack.pop_back();
                    break;
                }
                case OpCode::RETURN1: {
                    int a = GETARG_A(i);
                    if (call_stack.size() > 1) {
                        CallInfo& caller_frame = call_stack[call_stack.size() - 2];
                        Instruction call_i = *(caller_frame.pc - 1);
                        int result_reg = GETARG_A(call_i);
                        stack[caller_frame.stack_base + result_reg] = stack[frame->stack_base + a];
                        top = caller_frame.stack_base + result_reg + 1;
                    } else {
                        stack[0] = stack[frame->stack_base + a];
                        top = 1;
                    }
                    call_stack.pop_back();
                    break;
                }
                case OpCode::FORLOOP: {
                    int a = GETARG_A(i);
                    int bx = GETARG_Bx(i);
                    
                    // R[A] += R[A+2]
                    LuaValue step = stack[frame->stack_base + a + 2];
                    LuaValue index = stack[frame->stack_base + a];
                    
                    if (index.getType() == LuaType::NUMBER && step.getType() == LuaType::NUMBER) {
                        luaNumber idx = get_number_from_value(index);
                        luaNumber stp = get_number_from_value(step);
                        luaNumber new_idx = idx + stp;
                        stack[frame->stack_base + a] = LuaValue(new LuaNumber(new_idx), LuaType::NUMBER);
                        
                        // Check if loop should continue
                        LuaValue limit = stack[frame->stack_base + a + 1];
                        if (limit.getType() == LuaType::NUMBER) {
                            luaNumber lim = get_number_from_value(limit);
                            if ((stp > 0 && new_idx <= lim) || (stp < 0 && new_idx >= lim)) {
                                pc -= bx; // Jump back
                            }
                        }
                    }
                    break;
                }
                case OpCode::FORPREP: {
                    int a = GETARG_A(i);
                    int bx = GETARG_Bx(i);
                    
                    // Check values and prepare counters
                    LuaValue init = stack[frame->stack_base + a];
                    LuaValue limit = stack[frame->stack_base + a + 1];
                    LuaValue step = stack[frame->stack_base + a + 2];
                    
                    if (init.getType() == LuaType::NUMBER && 
                        limit.getType() == LuaType::NUMBER && 
                        step.getType() == LuaType::NUMBER) {
                        
                        luaNumber init_val = get_number_from_value(init);
                        luaNumber limit_val = get_number_from_value(limit);
                        luaNumber step_val = get_number_from_value(step);
                        
                        // Check if loop should run
                        bool should_run = false;
                        if (step_val > 0) {
                            should_run = init_val <= limit_val;
                        } else if (step_val < 0) {
                            should_run = init_val >= limit_val;
                        } else {
                            should_run = false; // step == 0, infinite loop
                        }
                        
                        if (!should_run) {
                            pc += bx + 1; // Skip the loop
                        }
                    } else {
                        pc += bx + 1; // Skip the loop if values are not numbers
                    }
                    break;
                }
                case OpCode::CLOSURE: {
                    int a = GETARG_A(i);
                    int bx = GETARG_Bx(i);
                    LuaValue proto_val = func->getProtos()[bx];
                    if (proto_val.getType() == LuaType::FUNCTION) {
                        auto* proto = dynamic_cast<LuaFunction*>(proto_val.getObject());
                        LuaClosure* new_closure = new LuaClosure(proto);
                        // initialize captured upvalues
                        const auto& updescs = proto->getUpvalDescs();
                        auto& new_upvals = new_closure->getUpvalues();
                        new_upvals.reserve(updescs.size());
                        for (const auto& desc : updescs) {
                            UpValue* uv = nullptr;
                            if (desc.inStack) {
                                LuaValue* loc = &stack[frame->stack_base + desc.idx];
                                uv = new UpValue(loc);
                            } else {
                                auto& outer = frame->closure->getUpvalues();
                                if (desc.idx < 0 || desc.idx >= static_cast<int>(outer.size())) {
                                    throw std::runtime_error("Invalid upvalue index in CLOSURE");
                                }
                                LuaValue* loc = outer[desc.idx]->getLocation();
                                uv = new UpValue(loc);
                            }
                            uv->retain();
                            new_upvals.push_back(uv);
                        }
                        stack[frame->stack_base + a] = LuaValue(new_closure, LuaType::FUNCTION);
                    } else {
                        throw std::runtime_error("Attempt to create closure from non-function(proto) value");
                    }
                    break;
                }
                case OpCode::GETUPVAL: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    auto& upvals = frame->closure->getUpvalues();
                    if (b < 0 || b >= static_cast<int>(upvals.size())) {
                        throw std::runtime_error("GETUPVAL: invalid upvalue index");
                    }
                    stack[frame->stack_base + a] = upvals[b]->getValue();
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::SETUPVAL: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    auto& upvals = frame->closure->getUpvalues();
                    if (b < 0 || b >= static_cast<int>(upvals.size())) {
                        throw std::runtime_error("SETUPVAL: invalid upvalue index");
                    }
                    upvals[b]->setValue(stack[frame->stack_base + a]);
                    break;
                }
                case OpCode::MMBIN: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    const LuaValue& key = mm_key_from_C(c);
                    LuaValue va = stack[frame->stack_base + a];
                    LuaValue vb = stack[frame->stack_base + b];
                    auto try_mm = [&](const LuaValue& v1, const LuaValue& v2) -> bool {
                        if (auto* gc = dynamic_cast<LuaGCObject*>(v1.getObject())) {
                            LuaValue mmf = gc->getMetamethod(key);
                            if (mmf.getType() == LuaType::FUNCTION) {
                                if (try_call_c_metamethod(*this, frame, a, mmf, v1, v2)) { return true; }
                            }
                        }
                        return false;
                    };
                    if (!(try_mm(va, vb) || try_mm(vb, va))) {
                        std::cerr << "metamethod not found for MMBIN" << std::endl;
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::MMBINI: {
                    int a = GETARG_A(i);
                    int sb = GETARG_sB(i);
                    int c = GETARG_C(i);
                    const LuaValue& key = mm_key_from_C(c);
                    LuaValue va = stack[frame->stack_base + a];
                    LuaValue vb(new LuaInteger(sb), LuaType::NUMBER);
                    auto try_mm = [&](const LuaValue& v1, const LuaValue& v2) -> bool {
                        if (auto* gc = dynamic_cast<LuaGCObject*>(v1.getObject())) {
                            LuaValue mmf = gc->getMetamethod(key);
                            if (mmf.getType() == LuaType::FUNCTION) {
                                if (try_call_c_metamethod(*this, frame, a, mmf, v1, v2)) { return true; }
                            }
                        }
                        return false;
                    };
                    if (!(try_mm(va, vb) || try_mm(vb, va))) {
                        std::cerr << "metamethod not found for MMBINI" << std::endl;
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::MMBINK: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    const LuaValue& key = mm_key_from_C(c);
                    LuaValue va = stack[frame->stack_base + a];
                    LuaValue vb = func->getConstants()[b];
                    auto try_mm = [&](const LuaValue& v1, const LuaValue& v2) -> bool {
                        if (auto* gc = dynamic_cast<LuaGCObject*>(v1.getObject())) {
                            LuaValue mmf = gc->getMetamethod(key);
                            if (mmf.getType() == LuaType::FUNCTION) {
                                if (try_call_c_metamethod(*this, frame, a, mmf, v1, v2)) { return true; }
                            }
                        }
                        return false;
                    };
                    if (!(try_mm(va, vb) || try_mm(vb, va))) {
                        std::cerr << "metamethod not found for MMBINK" << std::endl;
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::TFORPREP: {
                    int a = GETARG_A(i);
                    int bx = GETARG_Bx(i);
                    // Create upvalue for R[A + 3]
                    // This is a simplified implementation
                    // In a full implementation, we would create an upvalue for the iterator state
                    pc += bx;
                    break;
                }
                case OpCode::TFORCALL: {
                    int a = GETARG_A(i);
                    int c = GETARG_C(i);
                    // R[A+4], ... ,R[A+3+C] := R[A](R[A+1], R[A+2])
                    LuaValue iterator = stack[frame->stack_base + a];
                    LuaValue state = stack[frame->stack_base + a + 1];
                    LuaValue control = stack[frame->stack_base + a + 2];
                    
                    if (iterator.getType() == LuaType::FUNCTION) {
                        // Call the iterator function
                        // This is a simplified implementation
                        // In a full implementation, we would call the iterator and store results
                        for (int j = 0; j < c; j++) {
                            stack[frame->stack_base + a + 4 + j] = LuaValue(); // nil for now
                        }
                    }
                    break;
                }
                case OpCode::TFORLOOP: {
                    int a = GETARG_A(i);
                    int bx = GETARG_Bx(i);
                    // if R[A+2] ~= nil then { R[A]=R[A+2]; pc -= Bx }
                    LuaValue control = stack[frame->stack_base + a + 2];
                    if (control.getType() != LuaType::NIL) {
                        stack[frame->stack_base + a] = control;
                        pc -= bx;
                    }
                    break;
                }
                case OpCode::SETLIST: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    // R[A][C+i] := R[A+i], 1 <= i <= B
                    LuaValue table = stack[frame->stack_base + a];
                    if (table.getType() == LuaType::TABLE) {
                        if (auto* tbl = dynamic_cast<LuaTable*>(table.getObject())) {
                            for (int j = 1; j <= b; j++) {
                                LuaValue value = stack[frame->stack_base + a + j];
                                tbl->set(c + j, value);
                            }
                        }
                    }
                    break;
                }
                case OpCode::VARARG: {
                    int a = GETARG_A(i);
                    int c = GETARG_C(i);
                    // R[A], R[A+1], ..., R[A+C-2] = vararg
                    // Get vararg values from the function's vararg list
                    LuaFunction* func = frame->closure->getFunction();
                    const auto& varargs = func->getVarargs();
                    int num_vars = std::min(c - 1, static_cast<int>(varargs.size()));
                    
                    for (int j = 0; j < num_vars; j++) {
                        stack[frame->stack_base + a + j] = varargs[j];
                    }
                    // Fill remaining with nil if needed
                    for (int j = num_vars; j < c - 1; j++) {
                        stack[frame->stack_base + a + j] = LuaValue(); // nil
                    }
                    top = frame->stack_base + a + c - 1;
                    break;
                }
                case OpCode::VARARGPREP: {
                    int a = GETARG_A(i);
                    // Adjust vararg parameters
                    // This is a simplified implementation
                    // In a full implementation, we would adjust the vararg parameters
                    // For now, we just ensure the stack has enough space
                    break;
                }
                case OpCode::EXTRAARG: {
                    // Extra (larger) argument for previous opcode
                    // This is handled by the previous opcode
                    break;
                }
                default: {
                    std::cout << "Unknown opcode: " << to_string(op) << std::endl;
                    return;
                }
            }
            // If we are here, it means we have returned from a function or called one.
            // so break from the inner loop to get the new frame.
            if (op == OpCode::CALL || op == OpCode::TAILCALL || op == OpCode::RETURN || op == OpCode::RETURN0 || op == OpCode::RETURN1) {
                break;
            }
        }
    }
}

} // namespace luao
