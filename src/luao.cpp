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

int main(int argc, char **argv)
{
    try {
        std::cout << "Starting tests..." << std::endl;
        test_cfunction_call();
        std::cout << "CFunction test passed." << std::endl;
        
        std::cout << "All tests passed." << std::endl;
    } catch (const std::runtime_error e) {
        std::cout << "Test Failed: " << e.what() << std::endl;
    } catch (const std::exception e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    return 0;
}