#include "vm.hpp"
#include "debug.hpp"
#include <table.hpp>
#include <iostream>

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

VM::VM() : top(0) {
    stack.resize(256);
}

void VM::load(LuaFunction* main_function) {
    main_function_ = LuaValue(main_function, LuaType::FUNCTION);
    call_stack.clear();
    stack.clear();
    stack.resize(256);
    top = 0;

    call_stack.emplace_back(main_function, &main_function->getBytecode()[0], 0);
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

void VM::run() {
    while (!call_stack.empty()) {
        CallFrame* frame = &call_stack.back();
        const Instruction* pc = frame->pc;

        for (;;) {
            Instruction i = *pc++;
            if (trace_execution) {
                std::cout << disassemble_instruction(i, frame->func) << std::endl;
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
                    stack[frame->stack_base + a] = frame->func->getConstants()[bx];
                    top = frame->stack_base + a + 1;
                    break;
                }
                // case OpCode::LOADKX: {}
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
                // case OpCode::GETTABUP: {}
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
                    LuaValue k = frame->func->getConstants()[c];

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
                // case OpCode::SETTABUP: {}
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
                    LuaValue k = frame->func->getConstants()[b];
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
                // case OpCode::SELF: {}
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
                        // Metamethods would be handled here
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::ADD:
                case OpCode::SUB:
                case OpCode::MUL:
                case OpCode::DIV: {
                    int a = GETARG_A(i); /* args are 'A B C' */
                    int b = GETARG_B(i);
                    int c = GETARG_C(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    LuaValue rc = stack[frame->stack_base + c];

                    if (rb.getType() == LuaType::NUMBER && rc.getType() == LuaType::NUMBER) {
                        auto* int_b = dynamic_cast<LuaInteger*>(rb.getObject());
                        auto* int_c = dynamic_cast<LuaInteger*>(rc.getObject());

                        if (int_b && int_c && op != OpCode::DIV) {
                            luaInt val_b = int_b->getValue();
                            luaInt val_c = int_c->getValue();
                            luaInt res;
                            if (op == OpCode::ADD) res = val_b + val_c;
                            else if (op == OpCode::SUB) res = val_b - val_c;
                            else res = val_b * val_c;
                            stack[frame->stack_base + a] = LuaValue(new LuaInteger(res), LuaType::NUMBER);
                        } else {
                            luaNumber nb = get_number_from_value(rb);
                            luaNumber nc = get_number_from_value(rc);
                            luaNumber res;
                            if (op == OpCode::ADD) res = nb + nc;
                            else if (op == OpCode::SUB) res = nb - nc;
                            else if (op == OpCode::MUL) res = nb * nc;
                            else res = nb / nc;
                            stack[frame->stack_base + a] = LuaValue(new LuaNumber(res), LuaType::NUMBER);
                        }
                    } else {
                        // Metamethods would be handled here
                    }
                    top = frame->stack_base + a + 1;
                    break;
                }
                case OpCode::UNM:
                case OpCode::BNOT: {
                    int a = GETARG_A(i); /* args are 'A B' */
                    int b = GETARG_B(i);
                    LuaValue rb = stack[frame->stack_base + b];
                    std::string err = op == OpCode::UNM ? "arithmetic" : "bitwise";
                    if (rb.getType() == LuaType::NUMBER) {
                        if (auto* int_b = dynamic_cast<LuaInteger*>(rb.getObject())) {
                            luaInt res = int_b->getValue();
                            if (op == OpCode::UNM) res = -res; else res = ~res;
                            stack[frame->stack_base + a] = LuaValue(new LuaInteger(res), LuaType::NUMBER);
                        } else if (op == OpCode::UNM) {
                            auto* num_b = dynamic_cast<LuaNumber*>(rb.getObject());
                            luaNumber res = -num_b->getValue();
                            stack[frame->stack_base + a] = LuaValue(new LuaNumber(res), LuaType::NUMBER);
                        } else {
                            std::cerr << "attempt to perform " << err << " operation on a " << rb.typeName() << " value" << std::endl;
                        }
                    } else {
                        std::cerr << "attempt to perform " << err << " operation on a " << rb.typeName() << " value" << std::endl;
                    }
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
                    if (rb.getType() == LuaType::STRING) {
                        auto* str = dynamic_cast<LuaString*>(rb.getObject());
                        stack[frame->stack_base + a] = LuaValue(new LuaInteger(str->getValue().size()), LuaType::NUMBER);
                    } else if (rb.getType() == LuaType::TABLE) {
                        auto* table = dynamic_cast<LuaTable*>(rb.getObject());
                        LuaValue mm = table->getMetamethod(mm::__len);
                        if (mm.getObject()) { /* metamethod */ }
                        else stack[frame->stack_base + a] = table->vlen();
                    } else {
                         if (auto* gc = dynamic_cast<LuaGCObject*>(rb.getObject())) {
                             LuaValue mm = gc->getMetamethod(mm::__len);
                             if(mm.getObject()) { /* metamethod */ }
                             else std::cerr << "attempt to get length of a " << rb.typeName() << " value" << std::endl;
                         } else {
                            std::cerr << "attempt to get length of a " << rb.typeName() << " value" << std::endl;
                         }
                    }
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
                        std::ostringstream oss;
                        for (int j = b; j <= c; j++) {
                            oss << stack[frame->stack_base + j].getObject()->toString();
                        }
                        stack[frame->stack_base + a] = LuaValue(new LuaString(oss.str()), LuaType::STRING);
                    }
                    top = frame->stack_base + a + 1;
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
                        auto* func = dynamic_cast<LuaFunction*>(func_val.getObject());
                        frame->pc = pc;
                        call_stack.emplace_back(func, &func->getBytecode()[0], frame->stack_base + a + 1);
                        break; // Continue to the new frame's execution loop
                    } else {
                        std::cerr << "Attempt to call a " << func_val.typeName() << " value" << std::endl;
                        return; // or error
                    }
                }
                case OpCode::RETURN: {
                    int a = GETARG_A(i);
                    int b = GETARG_B(i);
                    int n_results = b - 1;
                    if (n_results < 0) n_results = top - (frame->stack_base + a); // variadic results

                    int caller_base = call_stack.size() > 1 ? call_stack[call_stack.size() - 2].stack_base : 0;

                    for (int j = 0; j < n_results; j++) {
                        stack[caller_base + j] = stack[frame->stack_base + a + j];
                    }
                    top = caller_base + n_results;
                    call_stack.pop_back();
                    break;
                }
                case OpCode::RETURN0: {
                    top = call_stack.size() > 1 ? call_stack[call_stack.size() - 2].stack_base : 0;
                    call_stack.pop_back();
                    break;
                }
                case OpCode::RETURN1: {
                    int a = GETARG_A(i);
                    int caller_base = call_stack.size() > 1 ? call_stack[call_stack.size() - 2].stack_base : 0;
                    stack[caller_base] = stack[frame->stack_base + a];
                    top = caller_base + 1;
                    call_stack.pop_back();
                    break;
                }
                default: {
                    std::cout << "Unknown opcode: " << to_string(op) << std::endl;
                    return;
                }
            }
            // If we are here, it means we have returned from a function or called one.
            // so break from the inner loop to get the new frame.
            if (op == OpCode::CALL || op == OpCode::RETURN || op == OpCode::RETURN0 || op == OpCode::RETURN1) {
                break;
            }
        }
    }
}

} // namespace luao
