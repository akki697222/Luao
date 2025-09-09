#include <iostream>
#include <vector>
#include <cassert>
#include "vm.hpp"
#include "opcodes.hpp"
#include "object.hpp"
#include "function.hpp"
#include "closure.hpp"

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
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
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
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
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
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
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
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
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
    LuaClosure* main_closure = new LuaClosure(main_func);
    VM vm;
    vm.load(main_closure);
    vm.set_trace(true);
    vm.run();
    LuaValue result = vm.get_stack_top();
    assert(result.getType() == LuaType::NUMBER);
    auto* int_res = dynamic_cast<LuaInteger*>(result.getObject());
    assert(int_res != nullptr);
    assert(int_res->getValue() == 300);
    std::cout << "LOADK test passed." << std::endl;
}

void test_closure() {
    std::cout << "--- Testing CLOSURE ---" << std::endl;

    // Child function prototype
    std::vector<Instruction> child_bytecode = {
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 0, CREATE_sBx(42)), // Load a constant value
        CREATE_A(static_cast<int>(OpCode::RETURN1), 0)
    };
    std::vector<LuaValue> child_constants = {};
    LuaFunction* child_proto = new LuaFunction(child_bytecode, child_constants);

    // Parent function
    std::vector<Instruction> parent_bytecode = {
        CREATE_ABx(static_cast<int>(OpCode::CLOSURE), 0, 0), // R0 = closure(K0)
        CREATE_ABC(static_cast<int>(OpCode::CALL), 0, 1, 2), // R0 = R0()
        CREATE_A(static_cast<int>(OpCode::RETURN1), 0)
    };
    std::vector<LuaValue> parent_constants = {
        LuaValue(child_proto, LuaType::PROTOTYPE)
    };
    LuaFunction* parent_func = new LuaFunction(parent_bytecode, parent_constants);
    LuaClosure* parent_closure = new LuaClosure(parent_func);

    VM vm;
    vm.load(parent_closure);
    vm.set_trace(true);
    vm.run();

    LuaValue result = vm.get_stack_top();
    assert(result.getType() == LuaType::NUMBER);
    auto* int_res = dynamic_cast<LuaInteger*>(result.getObject());
    assert(int_res != nullptr);
    assert(int_res->getValue() == 42);

    std::cout << "CLOSURE test passed." << std::endl;
}


int main(int argc, char **argv)
{
    test_add();
    test_sub();
    test_mul();
    test_div();
    test_loadk();
    test_closure();

    std::cout << "All tests passed." << std::endl;

    return 0;
}