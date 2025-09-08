#include <iostream>
#include <vector>
#include <cassert>
#include "vm.hpp"
#include "opcodes.hpp"
#include "object.hpp"

using namespace luao;

#define CREATE_ABC(o, a, b, c)  ((static_cast<Instruction>(o) << 0) \
                        | (static_cast<Instruction>(a) << 7) \
                        | (static_cast<Instruction>(b) << 16) \
                        | (static_cast<Instruction>(c) << 24))

#define CREATE_ABx(o, a, bx)    ((static_cast<Instruction>(o) << 0) \
                        | (static_cast<Instruction>(a) << 7) \
                        | (static_cast<Instruction>(bx) << 15))

#define CREATE_sBx(i)   ((i) + 65535)

void test_add() {
    std::cout << "--- Testing ADD ---" << std::endl;
    std::vector<Instruction> bytecode = {
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 0, CREATE_sBx(1)),
        CREATE_ABx(static_cast<int>(OpCode::LOADI), 1, CREATE_sBx(2)),
        CREATE_ABC(static_cast<int>(OpCode::ADD), 2, 0, 1),
        CREATE_ABC(static_cast<int>(OpCode::RETURN), 2, 2, 0)
    };
    std::vector<LuaValue> constants = {};
    VM vm(bytecode, constants);
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
        CREATE_ABC(static_cast<int>(OpCode::RETURN), 2, 2, 0)
    };
    std::vector<LuaValue> constants = {};
    VM vm(bytecode, constants);
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
        CREATE_ABC(static_cast<int>(OpCode::RETURN), 2, 2, 0)
    };
    std::vector<LuaValue> constants = {};
    VM vm(bytecode, constants);
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
        CREATE_ABC(static_cast<int>(OpCode::RETURN), 2, 2, 0)
    };
    std::vector<LuaValue> constants = {};
    VM vm(bytecode, constants);
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
        CREATE_ABC(static_cast<int>(OpCode::RETURN), 2, 2, 0)
    };
    std::vector<LuaValue> constants = {
        LuaValue(new LuaInteger(100), LuaType::NUMBER),
        LuaValue(new LuaInteger(200), LuaType::NUMBER)
    };
    VM vm(bytecode, constants);
    vm.set_trace(true);
    vm.run();
    LuaValue result = vm.get_stack_top();
    assert(result.getType() == LuaType::NUMBER);
    auto* int_res = dynamic_cast<LuaInteger*>(result.getObject());
    assert(int_res != nullptr);
    assert(int_res->getValue() == 300);
    std::cout << "LOADK test passed." << std::endl;
}

int main(int argc, char **argv)
{
    test_add();
    test_sub();
    test_mul();
    test_div();
    test_loadk();

    std::cout << "All tests passed." << std::endl;

    return 0;
}