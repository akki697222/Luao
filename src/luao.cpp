#include <iostream>
#include <vector>
#include <cassert>
#include <vm.hpp>
#include <opcodes.hpp>
#include <object.hpp>   
#include <function.hpp>
#include <closure.hpp>
#include <table.hpp>

using namespace luao;

#define CREATE_ABC(o, a, b, c)  ((static_cast<Instruction>(o) << 0) \
                        | (static_cast<Instruction>(a) << 7) \
                        | (static_cast<Instruction>(b) << 16) \
                        | (static_cast<Instruction>(c) << 24))

#define CREATE_ABx(o, a, bx)    ((static_cast<Instruction>(o) << 0) \
                        | (static_cast<Instruction>(a) << 7) \
                        | (static_cast<Instruction>(bx) << 15))

#define CREATE_sBx(i)   ((i) + 65535)
#define CREATE_A(o, a)  ((static_cast<Instruction>(o) << 0) \
                        | (static_cast<Instruction>(a) << 7))

void test_cfunction_call() {
    std::cout << "--- Testing CFunction call ---" << std::endl;
    UpvalDesc _ENV; _ENV.name = "_ENV"; _ENV.inStack = true; _ENV.idx = 0;

    // C関数: 引数0個、"hello"を返す
    LuaNativeFunction* cprint = new LuaNativeFunction([](VM& vm, int base_reg, int num_args) -> int {
        auto& st = vm.get_stack_mutable();
        st[base_reg] = LuaValue(new LuaString("hello"), LuaType::STRING);
        return 1; // 戻り値1個
    });

    // main: R0 = cfunc; R0 = R0(); return
    std::vector<Instruction> bytecode = {
        CREATE_ABx(OpCode::LOADK, 0, 0),      /* R0 = K0 (cfunction) */
        CREATE_ABC(OpCode::CALL, 0, 1, 2),    /* R0 = R0() ; 1 ret */
        CREATE_A(OpCode::RETURN1, 0),         /* return R0 */
    };

    // 定数テーブルにcfunctionを格納
    std::vector<LuaValue> constants = {
        LuaValue(cprint, LuaType::FUNCTION)
    };

    LuaFunction* main_func = new LuaFunction(bytecode, constants, {}, {_ENV});
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
    vm.set_trace(true);
    vm.run();
    auto result = vm.get_stack_mutable()[0];
    assert(result.getType() == LuaType::STRING);
    std::cout << "Result: " << result.getObject()->toString() << std::endl;
}

void test_metamethod() {
    std::cout << "--- Testing Metamethod ---" << std::endl;
    UpvalDesc _ENV; _ENV.name = "_ENV"; _ENV.inStack = true; _ENV.idx = 0;

    LuaNativeFunction* __add = new LuaNativeFunction([](VM& vm, int base_reg, int num_args) -> int {
        auto& stack = vm.get_stack_mutable();
        stack[base_reg] = LuaValue(new LuaInteger(10), LuaType::NUMBER);
        return 1;
    });

    LuaTable* a = new LuaTable(); 
    LuaTable* a_mt = new LuaTable();
    a_mt->set(mm::__add, LuaValue(__add, LuaType::FUNCTION));
    a->setMetatable(a_mt);

    std::vector<Instruction> bytecode = {
        CREATE_ABx(OpCode::LOADK, 0, 0),      /* R0 = K0 (table a) */
        CREATE_ABC(OpCode::ADDI, 1, 0, 20),    /* R1 = a + 20 */
        CREATE_A(OpCode::RETURN1, 1),         /* return R1 */
    };

    std::vector<LuaValue> constants = {
        LuaValue(a, LuaType::TABLE)
    };

    LuaFunction* main_func = new LuaFunction(bytecode, constants, {}, {_ENV});
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
    vm.set_trace(true);
    vm.run();
    auto result = vm.get_stack_mutable()[0];
    assert(result.getType() == LuaType::NUMBER
        && dynamic_cast<LuaInteger*>(result.getObject())->getValue() == 10);
    std::cout << "Result: " << result.getObject()->toString() << std::endl;
}

void test_stack_overflow() {
    std::cout << "--- Testing Stack Overflow Detection ---" << std::endl;
    UpvalDesc _ENV; _ENV.name = "_ENV"; _ENV.inStack = true; _ENV.idx = 0;
    UpvalDesc _ENV_f; _ENV_f.name = "_ENV"; _ENV_f.inStack = false; _ENV_f.idx = 0;

    std::vector<Instruction> bytecode_f = {
        CREATE_ABC(OpCode::GETTABUP, 0, 0, 0),
        CREATE_ABC(OpCode::CALL, 0, 1, 1),
        static_cast<Instruction>(OpCode::RETURN0)
    };

    
    std::vector<LuaValue> constants_f = {
        LuaValue(new LuaString("func"), LuaType::STRING)
    };


    LuaFunction* func = new LuaFunction(bytecode_f, constants_f, {}, {_ENV_f});

    std::vector<Instruction> bytecode = {
        CREATE_ABx(OpCode::CLOSURE, 0, 0),
        CREATE_ABC(OpCode::SETTABUP, 0, 0, 0),
        CREATE_ABC(OpCode::GETTABUP, 0, 0, 0),
        CREATE_ABC(OpCode::CALL, 0, 1, 1),
        CREATE_ABC(OpCode::RETURN, 0, 1, 1)
    };

    std::vector<LuaValue> constants = {
        LuaValue(new LuaString("func"), LuaType::STRING)
    };

    std::vector<LuaValue> protos = {
        LuaValue(func, LuaType::FUNCTION)
    };

    LuaFunction* main_func = new LuaFunction(bytecode, constants, protos, {_ENV});
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
    vm.set_trace(true);
    vm.run();
}

int main(int argc, char **argv)
{
    try {
        std::cout << "Starting tests..." << std::endl;
        test_cfunction_call();
        std::cout << "CFunction test passed." << std::endl;
        test_metamethod();
        std::cout << "Metamethod test passed." << std::endl;
        test_stack_overflow();
        
        std::cout << "All tests passed." << std::endl;
    } catch (const std::runtime_error& e) {
        std::cout << "Test Failed: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    return 0;
}