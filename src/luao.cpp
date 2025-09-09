#include <iostream>
#include <vector>
#include <cassert>
#include "vm.hpp"
#include "opcodes.hpp"
#include "object.hpp"
#include "function.hpp"

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

void test_add() {
    std::cout << "--- Testing ADD ---" << std::endl;
    std::vector<Instruction> bytecode = {
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 0, CREATE_sBx(1)),
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 1, CREATE_sBx(2)),
        CREATE_ABC(static_cast<int>(OpCode::ADD), 2, 0, 1),
        CREATE_A(static_cast<int>(OpCode::RETURN1), 2)
    };
    std::vector<LuaValue> constants = {};
    LuaFunction* main_func = new LuaFunction(bytecode, constants);
    VM vm;
    vm.load(main_func);
    vm.set_trace(true);
    vm.run();
    LuaValue result = vm.get_stack_top();
    assert(result.getType() == LuaType::NUMBER);
    auto* int_res = dynamic_cast<LuaInteger*>(result.getObject());
    assert(int_res != nullptr);
    assert(int_res->getValue() == 3);
    std::cout << "ADD test passed." << std::endl;
}

void test_sub() {
    std::cout << "--- Testing SUB ---" << std::endl;
    std::vector<Instruction> bytecode = {
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 0, CREATE_sBx(10)),
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 1, CREATE_sBx(4)),
        CREATE_ABC(static_cast<int>(OpCode::SUB), 2, 0, 1),
        CREATE_A(static_cast<int>(OpCode::RETURN1), 2)
    };
    std::vector<LuaValue> constants = {};
    LuaFunction* main_func = new LuaFunction(bytecode, constants);
    VM vm;
    vm.load(main_func);
    vm.set_trace(true);
    vm.run();
    LuaValue result = vm.get_stack_top();
    assert(result.getType() == LuaType::NUMBER);
    auto* int_res = dynamic_cast<LuaInteger*>(result.getObject());
    assert(int_res != nullptr);
    assert(int_res->getValue() == 6);
    std::cout << "SUB test passed." << std::endl;
}

void test_mul() {
    std::cout << "--- Testing MUL ---" << std::endl;
    std::vector<Instruction> bytecode = {
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 0, CREATE_sBx(10)),
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 1, CREATE_sBx(4)),
        CREATE_ABC(static_cast<int>(OpCode::MUL), 2, 0, 1),
        CREATE_A(static_cast<int>(OpCode::RETURN1), 2)
    };
    std::vector<LuaValue> constants = {};
    LuaFunction* main_func = new LuaFunction(bytecode, constants);
    VM vm;
    vm.load(main_func);
    vm.set_trace(true);
    vm.run();
    LuaValue result = vm.get_stack_top();
    assert(result.getType() == LuaType::NUMBER);
    auto* int_res = dynamic_cast<LuaInteger*>(result.getObject());
    assert(int_res != nullptr);
    assert(int_res->getValue() == 40);
    std::cout << "MUL test passed." << std::endl;
}

void test_div() {
    std::cout << "--- Testing DIV ---" << std::endl;
    std::vector<Instruction> bytecode = {
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 0, CREATE_sBx(10)),
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 1, CREATE_sBx(4)),
        CREATE_ABC(static_cast<int>(OpCode::DIV), 2, 0, 1),
        CREATE_A(static_cast<int>(OpCode::RETURN1), 2)
    };
    std::vector<LuaValue> constants = {};
    LuaFunction* main_func = new LuaFunction(bytecode, constants);
    VM vm;
    vm.load(main_func);
    vm.set_trace(true);
    vm.run();
    LuaValue result = vm.get_stack_top();
    assert(result.getType() == LuaType::NUMBER);
    auto* float_res = dynamic_cast<LuaNumber*>(result.getObject());
    assert(float_res != nullptr);
    assert(float_res->getValue() == 2.5);
    std::cout << "DIV test passed." << std::endl;
}

void test_loadk() {
    std::cout << "--- Testing LOADK ---" << std::endl;
    std::vector<Instruction> bytecode = {
        CREATE_ABx(static_cast<int>(OpCode::LOADK), 0, 0),
        CREATE_ABx(static_cast<int>(OpCode::LOADK), 1, 1),
        CREATE_ABC(static_cast<int>(OpCode::ADD), 2, 0, 1),
        CREATE_A(static_cast<int>(OpCode::RETURN1), 2)
    };
    std::vector<LuaValue> constants = {
        LuaValue(new LuaInteger(100), LuaType::NUMBER),
        LuaValue(new LuaInteger(200), LuaType::NUMBER)
    };
    LuaFunction* main_func = new LuaFunction(bytecode, constants);
    VM vm;
    vm.load(main_func);
    vm.set_trace(true);
    vm.run();
    LuaValue result = vm.get_stack_top();
    assert(result.getType() == LuaType::NUMBER);
    auto* int_res = dynamic_cast<LuaInteger*>(result.getObject());
    assert(int_res != nullptr);
    assert(int_res->getValue() == 300);
    std::cout << "LOADK test passed." << std::endl;
}

void test_call() {
    std::cout << "--- Testing CALL ---" << std::endl;

    // Callee function: adds its first two arguments
    std::vector<Instruction> add_func_bytecode = {
        // args are at stack base 0 and 1
        CREATE_ABC(static_cast<int>(OpCode::ADD), 2, 0, 1), // R2 = R0 + R1
        CREATE_A(static_cast<int>(OpCode::RETURN1), 2) // return R2
    };
    std::vector<LuaValue> add_func_constants = {};
    LuaFunction* add_func = new LuaFunction(add_func_bytecode, add_func_constants);

    // Main function
    std::vector<Instruction> main_bytecode = {
        CREATE_ABx(static_cast<int>(OpCode::LOADK), 0, 0), // R0 = K0 (the add function)
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 1, CREATE_sBx(10)), // R1 = 10
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 2, CREATE_sBx(20)), // R2 = 20
        CREATE_ABC(static_cast<int>(OpCode::CALL), 0, 3, 2), // R0 = R0(R1, R2). 3->2 args (B-1), 2->1 result (C-1)
        CREATE_A(static_cast<int>(OpCode::RETURN1), 0) // return R0
    };
    std::vector<LuaValue> main_constants = {
        LuaValue(add_func, LuaType::FUNCTION)
    };
    LuaFunction* main_func = new LuaFunction(main_bytecode, main_constants);

    VM vm;
    vm.load(main_func);
    vm.set_trace(true);
    vm.run();

    LuaValue result = vm.get_stack_top();
    assert(result.getType() == LuaType::NUMBER);
    auto* int_res = dynamic_cast<LuaInteger*>(result.getObject());
    assert(int_res != nullptr);
    assert(int_res->getValue() == 30);

    std::cout << "CALL test passed." << std::endl;
}

int main(int argc, char **argv)
{
    test_add();
    test_sub();
    test_mul();
    test_div();
    test_loadk();
    test_call();

    std::cout << "All tests passed." << std::endl;

    return 0;
}