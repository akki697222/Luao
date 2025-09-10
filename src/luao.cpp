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
    auto top = vm.get_stack_top();
    assert(top.getType() == LuaType::STRING);
    std::cout << "Result: " << top.getObject()->toString() << std::endl;
}

void test_metamethod_add() {
    std::cout << "--- Testing Metamethod __add ---" << std::endl;
    UpvalDesc _ENV; _ENV.name = "_ENV"; _ENV.inStack = true; _ENV.idx = 0;

    // __add メタメソッド（C関数）: 2引数を受け取り、"mmadd"を返す
    LuaNativeFunction* mm_add = new LuaNativeFunction([](VM& vm, int base_reg, int num_args) -> int {
        auto& st = vm.get_stack_mutable();
        (void)num_args;
        st[base_reg] = LuaValue(new LuaString("mmadd"), LuaType::STRING);
        return 1;
    });

    // メタテーブルに __add を設定
    LuaTable* mt = new LuaTable();
    mt->set(LuaValue(new LuaString("__add"), LuaType::STRING), LuaValue(mm_add, LuaType::FUNCTION));

    // オペランドとなるテーブル2つ
    LuaTable* ta = new LuaTable();
    LuaTable* tb = new LuaTable();
    ta->setMetatable(mt);
    tb->setMetatable(mt);

    // main: R0=K0(ta), R1=K1(tb), R2 = R0 + R1 (metamethod), return R2
    std::vector<Instruction> bytecode = {
        CREATE_ABx(OpCode::LOADK, 0, 0),
        CREATE_ABx(OpCode::LOADK, 1, 1),
        CREATE_ABC(OpCode::ADD, 2, 0, 1),
        CREATE_A(OpCode::RETURN1, 2),
    };

    std::vector<LuaValue> constants = {
        LuaValue(ta, LuaType::TABLE),
        LuaValue(tb, LuaType::TABLE),
    };

    LuaFunction* main_func = new LuaFunction(bytecode, constants, {}, {_ENV});
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
    vm.set_trace(true);
    vm.run();
    auto top = vm.get_stack_top();
    if (top.getType() == LuaType::STRING) {
        std::cout << top.getObject()->toString() << std::endl;
    } else {
        std::cout << "Error: Expected string, got " << top.typeName() << std::endl;
    }
    std::cout << "Metamethod ADD Result: " << top.getObject()->toString() << std::endl;
}

void test_arithmetic_operations() {
    std::cout << "--- Testing Arithmetic Operations ---" << std::endl;
    UpvalDesc _ENV; _ENV.name = "_ENV"; _ENV.inStack = true; _ENV.idx = 0;

    // Test: a = 10, b = 3, result = a + b
    std::vector<Instruction> bytecode = {
        CREATE_ABx(OpCode::LOADI, 0, CREATE_sBx(10)),  // R0 = 10
        CREATE_ABx(OpCode::LOADI, 1, CREATE_sBx(3)),   // R1 = 3
        CREATE_ABC(OpCode::ADD, 2, 0, 1),              // R2 = R0 + R1
        CREATE_A(OpCode::RETURN1, 2),                  // return R2
    };

    std::vector<LuaValue> constants = {};
    LuaFunction* main_func = new LuaFunction(bytecode, constants, {}, {_ENV});
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
    vm.set_trace(false);  // Disable trace for now
    vm.run();
    auto top = vm.get_stack_top();
    if (top.getType() == LuaType::NUMBER) {
        std::cout << "10 + 3 = " << top.getObject()->toString() << std::endl;
    } else {
        std::cout << "Error: Expected number, got " << top.typeName() << std::endl;
    }
}

void test_bitwise_operations() {
    std::cout << "--- Testing Bitwise Operations ---" << std::endl;
    UpvalDesc _ENV; _ENV.name = "_ENV"; _ENV.inStack = true; _ENV.idx = 0;

    // Test: a = 10 (0b1010), b = 12 (0b1100), result = a & b
    std::vector<Instruction> bytecode = {
        CREATE_ABx(OpCode::LOADI, 0, CREATE_sBx(10)),  // R0 = 10
        CREATE_ABx(OpCode::LOADI, 1, CREATE_sBx(12)),  // R1 = 12
        CREATE_ABC(OpCode::BAND, 2, 0, 1),             // R2 = R0 & R1
        CREATE_A(OpCode::RETURN1, 2),                  // return R2
    };

    std::vector<LuaValue> constants = {};
    LuaFunction* main_func = new LuaFunction(bytecode, constants, {}, {_ENV});
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
    vm.set_trace(false);
    vm.run();
    auto top = vm.get_stack_top();
    if (top.getType() == LuaType::NUMBER) {
        std::cout << "10 & 12 = " << top.getObject()->toString() << std::endl;
    } else {
        std::cout << "Error: Expected number, got " << top.typeName() << std::endl;
    }
}

void test_comparison_operations() {
    std::cout << "--- Testing Comparison Operations ---" << std::endl;
    UpvalDesc _ENV; _ENV.name = "_ENV"; _ENV.inStack = true; _ENV.idx = 0;

    // Test: a = 5, b = 10, result = a < b
    std::vector<Instruction> bytecode = {
        CREATE_ABx(OpCode::LOADI, 0, CREATE_sBx(5)),   // R0 = 5
        CREATE_ABx(OpCode::LOADI, 1, CREATE_sBx(10)),  // R1 = 10
        CREATE_ABC(OpCode::LT, 0, 0, 1),               // if R0 < R1 then pc++ else skip
        CREATE_ABx(OpCode::LOADI, 2, CREATE_sBx(1)),   // R2 = 1 (true)
        CREATE_ABx(OpCode::JMP, 0, CREATE_sBx(1)),     // jump to next
        CREATE_ABx(OpCode::LOADI, 2, CREATE_sBx(0)),   // R2 = 0 (false)
        CREATE_A(OpCode::RETURN1, 2),                  // return R2
    };

    std::vector<LuaValue> constants = {};
    LuaFunction* main_func = new LuaFunction(bytecode, constants, {}, {_ENV});
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
    vm.set_trace(true);
    vm.run();
    auto top = vm.get_stack_top();
    assert(top.getType() == LuaType::NUMBER);
    std::cout << "5 < 10 = " << top.getObject()->toString() << std::endl;
}

void test_string_operations() {
    std::cout << "--- Testing String Operations ---" << std::endl;
    UpvalDesc _ENV; _ENV.name = "_ENV"; _ENV.inStack = true; _ENV.idx = 0;

    // Test: s1 = "Hello", s2 = "World", result = s1 .. s2
    std::vector<Instruction> bytecode = {
        CREATE_ABx(OpCode::LOADK, 0, 0),               // R0 = K0 ("Hello")
        CREATE_ABx(OpCode::LOADK, 1, 1),               // R1 = K1 ("World")
        CREATE_ABC(OpCode::CONCAT, 2, 0, 1),           // R2 = R0 .. R1
        CREATE_A(OpCode::RETURN1, 2),                  // return R2
    };

    std::vector<LuaValue> constants = {
        LuaValue(new LuaString("Hello"), LuaType::STRING),
        LuaValue(new LuaString("World"), LuaType::STRING),
    };

    LuaFunction* main_func = new LuaFunction(bytecode, constants, {}, {_ENV});
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
    vm.set_trace(true);
    vm.run();
    auto top = vm.get_stack_top();
    if (top.getType() == LuaType::STRING) {
        std::cout << top.getObject()->toString() << std::endl;
    } else {
        std::cout << "Error: Expected string, got " << top.typeName() << std::endl;
    }
    std::cout << "Hello .. World = " << top.getObject()->toString() << std::endl;
}

void test_function_calls() {
    std::cout << "--- Testing Function Calls ---" << std::endl;
    UpvalDesc _ENV; _ENV.name = "_ENV"; _ENV.inStack = true; _ENV.idx = 0;

    // Create a simple function that adds two numbers
    std::vector<Instruction> add_bytecode = {
        CREATE_ABC(OpCode::ADD, 0, 0, 1),              // R0 = R0 + R1
        CREATE_A(OpCode::RETURN1, 0),                  // return R0
    };

    LuaFunction* add_func = new LuaFunction(add_bytecode, {}, {}, {_ENV});
    LuaClosure* add_closure = new LuaClosure(add_func);

    // Main function that calls add(5, 3)
    std::vector<Instruction> main_bytecode = {
        CREATE_ABx(OpCode::LOADK, 0, 0),               // R0 = K0 (add function)
        CREATE_ABx(OpCode::LOADI, 1, CREATE_sBx(5)),   // R1 = 5
        CREATE_ABx(OpCode::LOADI, 2, CREATE_sBx(3)),   // R2 = 3
        CREATE_ABC(OpCode::CALL, 0, 3, 2),             // R0 = R0(R1, R2) ; 2 args, 1 ret
        CREATE_A(OpCode::RETURN1, 0),                  // return R0
    };

    std::vector<LuaValue> constants = {
        LuaValue(add_closure, LuaType::FUNCTION),
    };

    LuaFunction* main_func = new LuaFunction(main_bytecode, constants, {}, {_ENV});
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
    vm.set_trace(true);
    vm.run();
    auto top = vm.get_stack_top();
    assert(top.getType() == LuaType::NUMBER);
    std::cout << "add(5, 3) = " << top.getObject()->toString() << std::endl;
}

void test_vararg() {
    std::cout << "--- Testing VARARG ---" << std::endl;
    UpvalDesc _ENV; _ENV.name = "_ENV"; _ENV.inStack = true; _ENV.idx = 0;

    // Create a function that uses vararg
    std::vector<Instruction> vararg_bytecode = {
        CREATE_ABC(OpCode::VARARG, 0, 0, 4),           // R0, R1, R2, R3 = vararg (up to 3 args)
        CREATE_ABC(OpCode::ADD, 4, 0, 1),              // R4 = R0 + R1
        CREATE_ABC(OpCode::ADD, 4, 4, 2),              // R4 = R4 + R2
        CREATE_A(OpCode::RETURN1, 4),                  // return R4
    };

    LuaFunction* vararg_func = new LuaFunction(vararg_bytecode, {}, {}, {_ENV});
    // Set varargs: 1, 2, 3
    std::vector<LuaValue> varargs = {
        LuaValue(new LuaInteger(1), LuaType::NUMBER),
        LuaValue(new LuaInteger(2), LuaType::NUMBER),
        LuaValue(new LuaInteger(3), LuaType::NUMBER),
    };
    vararg_func->setVarargs(varargs);
    LuaClosure* vararg_closure = new LuaClosure(vararg_func);

    // Main function that calls vararg function
    std::vector<Instruction> main_bytecode = {
        CREATE_ABx(OpCode::LOADK, 0, 0),               // R0 = K0 (vararg function)
        CREATE_ABC(OpCode::CALL, 0, 1, 1),             // R0 = R0() ; 0 args, 1 ret
        CREATE_A(OpCode::RETURN1, 0),                  // return R0
    };

    std::vector<LuaValue> constants = {
        LuaValue(vararg_closure, LuaType::FUNCTION),
    };

    LuaFunction* main_func = new LuaFunction(main_bytecode, constants, {}, {_ENV});
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
    vm.set_trace(true);
    vm.run();
    auto top = vm.get_stack_top();
    assert(top.getType() == LuaType::NUMBER);
    std::cout << "vararg sum(1, 2, 3) = " << top.getObject()->toString() << std::endl;
}

int main(int argc, char **argv)
{
    try {
        std::cout << "Starting tests..." << std::endl;
        test_cfunction_call();
        std::cout << "CFunction test passed." << std::endl;
        
        test_metamethod_add();
        std::cout << "Metamethod test passed." << std::endl;
        
        test_arithmetic_operations();
        std::cout << "Arithmetic test passed." << std::endl;
        
        test_bitwise_operations();
        std::cout << "Bitwise test passed." << std::endl;
        
        std::cout << "All tests passed." << std::endl;
    } catch (std::runtime_error e) {
        std::cout << "Test Failed: " << e.what() << std::endl;
    } catch (std::exception e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    return 0;
}