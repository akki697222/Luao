#include "vm.hpp"
#include "debug.hpp"
#include <table.hpp>
#include <iostream>

// metamethod key shortcuts
namespace mm {
    static const luao::LuaValue __add       = LuaValue(new luao::LuaString("__add"),       LuaType::STRING);
    static const luao::LuaValue __sub       = LuaValue(new luao::LuaString("__sub"),       LuaType::STRING);
    static const luao::LuaValue __mul       = LuaValue(new luao::LuaString("__mul"),       LuaType::STRING);
    static const luao::LuaValue __div       = LuaValue(new luao::LuaString("__div"),       LuaType::STRING);
    static const luao::LuaValue __mod       = LuaValue(new luao::LuaString("__mod"),       LuaType::STRING);
    static const luao::LuaValue __pow       = LuaValue(new luao::LuaString("__pow"),       LuaType::STRING);
    static const luao::LuaValue __unm       = LuaValue(new luao::LuaString("__unm"),       LuaType::STRING);
    static const luao::LuaValue __len       = LuaValue(new luao::LuaString("__len"),       LuaType::STRING);
    static const luao::LuaValue __eq        = LuaValue(new luao::LuaString("__eq"),        LuaType::STRING);
    static const luao::LuaValue __lt        = LuaValue(new luao::LuaString("__lt"),        LuaType::STRING);
    static const luao::LuaValue __le        = LuaValue(new luao::LuaString("__le"),        LuaType::STRING);
    static const luao::LuaValue __index     = LuaValue(new luao::LuaString("__index"),     LuaType::STRING);
    static const luao::LuaValue __newindex  = LuaValue(new luao::LuaString("__newindex"),  LuaType::STRING);
    static const luao::LuaValue __call      = LuaValue(new luao::LuaString("__call"),      LuaType::STRING);
    static const luao::LuaValue __tostring  = LuaValue(new luao::LuaString("__tostring"),  LuaType::STRING);
    static const luao::LuaValue __concat    = LuaValue(new luao::LuaString("__concat"),    LuaType::STRING);
    static const luao::LuaValue __metatable = LuaValue(new luao::LuaString("__metatable"), LuaType::STRING);
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

VM::VM() : pc(nullptr), top(0) {
    stack.resize(256);
}

VM::VM(std::vector<Instruction> bytecode, std::vector<LuaValue> constants) : top(0) {
    load(std::move(bytecode), std::move(constants));
}

void VM::load(std::vector<Instruction> bytecode, std::vector<LuaValue> constants) {
    this->bytecode = std::move(bytecode);
    this->constants = std::move(constants);
    this->pc = &this->bytecode[0];
    stack.clear();
    stack.resize(256);
    this->top = 0;
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
    for (;;) {
        Instruction i = *pc;
        if (trace_execution) {
            std::cout << disassemble_instruction(i) << std::endl;
        }
        pc++;
        OpCode op = GET_OPCODE(i);

        switch (op) {
            case OpCode::MOVE: {
                int a = GETARG_A(i); /* args are 'A B' */
                int b = GETARG_B(i);
                stack[a] = stack[b];
                break;
            }
            case OpCode::LOADI: {
                int a = GETARG_A(i); /* args are 'A sBx' */
                int sbx = GETARG_sBx(i);
                stack[a] = LuaValue(new LuaInteger(sbx), LuaType::NUMBER);
                top = a + 1;
                break;
            }
            case OpCode::LOADF: {
                int a = GETARG_A(i); /* args are 'A sBx' */
                int sbx = GETARG_sBx(i);
                stack[a] = LuaValue(new LuaNumber(static_cast<luaNumber>(sbx)), LuaType::NUMBER);
                top = a + 1;
                break;
            }
            case OpCode::LOADK: {
                int a = GETARG_A(i); /* args are 'A Bx' */
                int bx = GETARG_Bx(i);
                stack[a] = constants[bx];
                top = a + 1;
                break;
            }
            // case OpCode::LOADKX: {}
            case OpCode::LOADFALSE: {
                int a = GETARG_A(i); /* args are 'A' */
                stack[a] = LuaValue(FALSE_OBJ, LuaType::BOOLEAN);
                top = a + 1;
                break;
            }
            case OpCode::LFALSESKIP: {
                int a = GETARG_A(i); /* args are 'A' */
                stack[a] = LuaValue(FALSE_OBJ, LuaType::BOOLEAN);
                top = a + 1;
                pc++; // skip next instruction
                break;
            }
            case OpCode::LOADTRUE: {
                int a = GETARG_A(i); /* args are 'A' */
                stack[a] = LuaValue(TRUE_OBJ, LuaType::BOOLEAN);
                top = a + 1;
                break;
            }
            case OpCode::LOADNIL: {
                int a = GETARG_A(i); /* args are 'A B' */
                int b = GETARG_B(i);
                for (int j = 0; j <= b; j++) {
                    stack[a + j] = LuaValue(); // nil
                }
                top = a + b + 1;
                break;
            }
            // case OpCode::GETUPVAL: {}
            // case OpCode::SETUPVAL: {}
            // case OpCode::GETTABUP: {}
            case OpCode::GETTABLE: {
                int a = GETARG_A(i);
                int b = GETARG_B(i);
                int c = GETARG_C(i);

                LuaValue t = stack[b];
                LuaValue k = stack[c];

                if (t.getType() == LuaType::TABLE) {
                    if (auto* table = dynamic_cast<LuaTable*>(t.getObject())) {
                        stack[a] = table->get(k);
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
            }
            case OpCode::GETI: {
                int a = GETARG_A(i);
                int b = GETARG_B(i);
                int c = GETARG_C(i);

                LuaValue t = stack[b];

                if (t.getType() == LuaType::TABLE) {
                    if (auto* table = dynamic_cast<LuaTable*>(t.getObject())) {
                        stack[a] = table->get(c);
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
            
                top = a + 1;
                break;
            }
            case OpCode::GETFIELD: {
                int a = GETARG_A(i);
                int b = GETARG_B(i);
                int c = GETARG_C(i);

                LuaValue t = stack[b];
                LuaValue k = constants[c];

                if (t.getType() == LuaType::TABLE) {
                    if (auto* table = dynamic_cast<LuaTable*>(t.getObject())) {
                        stack[a] = table->get(k);
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
            }
            // case OpCode::SETTABUP: {}
            case OpCode::SETTABLE: {
                int a = GETARG_A(i);
                int b = GETARG_B(i);
                int c = GETARG_C(i); 
                        
                LuaValue t = stack[a];
                LuaValue k = stack[b]; 
                LuaValue v = stack[c];
                        
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

                LuaValue t = stack[a];
                LuaValue v = stack[c];

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
                        
                LuaValue t = stack[a];
                LuaValue k = constants[b]; 
                LuaValue v = stack[c];
                        
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
                int b = GETARG_B(i); /* Array initial size, unused */
                int c = GETARG_C(i); /* Hash initial size, unused */
                /* k is extension flag, unused for now */
                stack[a] = LuaValue(new LuaTable(), LuaType::TABLE);
                top = a + 1;
                break;
            }
            // case OpCode::SELF: {}
            case OpCode::ADDI: {
                int a = GETARG_A(i); /* args are 'A B sC' */
                int b = GETARG_B(i);
                int sc = GETARG_sC(i);
                LuaValue rb = stack[b];

                if (rb.getType() == LuaType::NUMBER) {
                    if (auto* int_b = dynamic_cast<LuaInteger*>(rb.getObject())) {
                        luaInt val_b = int_b->getValue();
                        luaInt res = val_b + sc;
                        stack[a] = LuaValue(new LuaInteger(res), LuaType::NUMBER);
                    } else if (auto* num_b = dynamic_cast<LuaNumber*>(rb.getObject())) {
                        luaNumber nb = num_b->getValue();
                        luaNumber res = nb + static_cast<luaNumber>(sc);
                        stack[a] = LuaValue(new LuaNumber(res), LuaType::NUMBER);
                    }
                } else {
                    // Metamethods would be handled here
                }
                top = a + 1;
                break;
            }
            // case OpCode::ADDK: {}
            // case OpCode::SUBK: {}
            // case OpCode::MULK: {}
            // case OpCode::MODK: {}
            // case OpCode::POWK: {}
            // case OpCode::DIVK: {}
            // case OpCode::IDIVK: {}
            // case OpCode::BANDK: {}
            // case OpCode::BORK: {}
            // case OpCode::BXORK: {}
            // case OpCode::SHRI: {}
            // case OpCode::SHLI: {}
            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::DIV: {
                int a = GETARG_A(i); /* args are 'A B C' */
                int b = GETARG_B(i);
                int c = GETARG_C(i);
                LuaValue rb = stack[b];
                LuaValue rc = stack[c];

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
                        stack[a] = LuaValue(new LuaInteger(res), LuaType::NUMBER);
                    } else {
                        luaNumber nb = get_number_from_value(rb);
                        luaNumber nc = get_number_from_value(rc);
                        luaNumber res;
                        if (op == OpCode::ADD) res = nb + nc;
                        else if (op == OpCode::SUB) res = nb - nc;
                        else if (op == OpCode::MUL) res = nb * nc;
                        else res = nb / nc;
                        stack[a] = LuaValue(new LuaNumber(res), LuaType::NUMBER);
                    }
                } else {
                    // Metamethods would be handled here

                }
                top = a + 1;
                break;
            }
            // case OpCode::MOD: {}
            // case OpCode::POW: {}
            // case OpCode::IDIV: {}
            // case OpCode::BAND: {}
            // case OpCode::BOR: {}
            // case OpCode::BXOR: {}
            // case OpCode::SHL: {}
            // case OpCode::SHR: {}
            // case OpCode::MMBIN: {}
            // case OpCode::MMBINI: {}
            // case OpCode::MMBINK: {}
            case OpCode::UNM:
            case OpCode::BNOT: {
                int a = GETARG_A(i); /* args are 'A B' */
                int b = GETARG_B(i);

                LuaValue rb = stack[b];

                std::string err = op == OpCode::UNM ? "arithmetic" : "bitwise";
                if (rb.getType() == LuaType::NUMBER) {
                    if (auto* int_b = dynamic_cast<LuaInteger*>(rb.getObject())) {
                        luaInt res = int_b->getValue();
                        if (op == OpCode::UNM) {
                            res = -res;
                        } else {
                            res = ~res;
                        }
                        stack[a] = LuaValue(new LuaInteger(res), LuaType::NUMBER);
                    } else if (op == OpCode::UNM) {
                        auto* num_b = dynamic_cast<LuaNumber*>(rb.getObject());
                        luaNumber res = -num_b->getValue();
                        stack[a] = LuaValue(new LuaNumber(res), LuaType::NUMBER);
                    } else {
                        std::cerr << "attempt to perform " << err << " operation on a " << rb.typeName() << " value" << std::endl;
                    }
                } else {
                    std::cerr << "attempt to perform " << err << " operation on a " << rb.typeName() << " value" << std::endl;
                }
            
                top = a + 1;
                break;
            }
            case OpCode::NOT: {
                int a = GETARG_A(i); /* args are 'A B' */
                int b = GETARG_B(i);
            
                stack[a] = LuaValue(as_bool(stack[b]) ? FALSE_OBJ : TRUE_OBJ, LuaType::BOOLEAN);
                top = a + 1;
                break;
            }
            case OpCode::LEN: {
                int a = GETARG_A(i); /* args are 'A B' */
                int b = GETARG_B(i);

                LuaValue rb = stack[b];

                if (rb.getType() == LuaType::STRING) {
                    auto* str = dynamic_cast<LuaString*>(rb.getObject());
                    stack[a] = LuaValue(new LuaInteger(str->getValue().size()), LuaType::NUMBER);
                } else if (rb.getType() == LuaType::TABLE) {
                    auto* table = dynamic_cast<LuaTable*>(rb.getObject());
                    // metamethod
                    LuaValue mm = table->getMetamethod(mm::__len);
                    if (mm.getObject()) {
                        
                    } else {
                        stack[a] = table->vlen();
                    }
                } else {
                    if (auto* gc = dynamic_cast<LuaGCObject*>(rb.getObject())) {
                        // metamethod
                        LuaValue mm = gc->getMetamethod(mm::__len);
                    } else {
                        std::cerr << "attempt to get length of a " << rb.typeName() << " value" << std::endl;
                    }
                }
            
                top = a + 1;
                break;
            }
            case OpCode::CONCAT: {
                int a = GETARG_A(i); /* args are 'A B C' */
                int b = GETARG_B(i);
                int c = GETARG_C(i);
                if (b > c) {
                    stack[a] = LuaValue(new LuaString(""), LuaType::STRING);
                } else {
                    std::ostringstream oss;
                    for (int j = b; j <= c; j++) {
                        LuaValue val = stack[j];
                        if (val.getType() == LuaType::STRING) {
                            auto* str_obj = dynamic_cast<LuaString*>(val.getObject());
                            if (str_obj) {
                                oss << str_obj->getValue();
                            } else {
                                oss << val.getObject()->toString();
                            }
                        } else if (val.getType() == LuaType::NUMBER) {
                            luaNumber num = get_number_from_value(val);
                            oss << num;
                        } else if (val.getType() == LuaType::BOOLEAN) {
                            oss << (val.getObject() == TRUE_OBJ ? "true" : "false");
                        } else if (val.getType() == LuaType::NIL) {
                            oss << "nil";
                        } else {
                            oss << val.getObject()->toString();
                        }
                    }
                    stack[a] = LuaValue(new LuaString(oss.str()), LuaType::STRING);
                }
                top = a + 1;
                break;
            }
            // case OpCode::CLOSE: {}
            // case OpCode::TBC: {}
            case OpCode::JMP: {
                // J = A
                int sJ = GETARG_sA(i);
                pc += sJ - 1;
                if (trace_execution) {
                    std::cout << "VM: JMP: Jumping to pc " << pc << std::endl;
                }
            }
            // case OpCode::EQ: {}
            // case OpCode::LT: {}
            // case OpCode::LE: {}
            // case OpCode::EQK: {}
            // case OpCode::EQI: {}
            // case OpCode::LTI: {}
            // case OpCode::LEI: {}
            // case OpCode::GTI: {}
            // case OpCode::GEI: {}
            // case OpCode::TEST: {}
            // case OpCode::TESTSET: {}
            // case OpCode::CALL: {}
            // case OpCode::TAILCALL: {}
            case OpCode::RETURN: {
                int a = GETARG_A(i); /* args are 'A B C k' */
                int b = GETARG_B(i);
                if (b == 1) { // 0 results
                    top = 0;
                } else if (b > 1) { // b-1 results
                    for (int j = 0; j < b - 1; j++) {
                        stack[j] = stack[a + j];
                    }
                    top = b - 1;
                }
                return;
            }
            case OpCode::RETURN0: {
                /* args are none */
                top = 0;
                return;
            }
            case OpCode::RETURN1: {
                int a = GETARG_A(i); /* args are 'A' */
                stack[0] = stack[a];
                top = 1;
                return;
            }
            // case OpCode::FORLOOP: {}
            // case OpCode::FORPREP: {}
            // case OpCode::TFORPREP: {}
            // case OpCode::TFORCALL: {}
            // case OpCode::TFORLOOP: {}
            // case OpCode::SETLIST: {}
            // case OpCode::CLOSURE: {}
            // case OpCode::VARARG: {}
            // case OpCode::VARARGPREP: {}
            // case OpCode::EXTRAARG: {}
            default: {
                std::cout << "Unknown opcode: " << to_string(op) << std::endl;
                return;
            }
        }
    }
}

} // namespace luao
