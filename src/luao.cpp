#include <iostream>
#include <vector>
#include <cassert>
#include <vm.hpp>
#include <opcodes.hpp>
#include <object.hpp>   
#include <function.hpp>
#include <closure.hpp>
#include <table.hpp>
#include <memory>

using namespace luao;

VM vm;

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
    std::shared_ptr<LuaNativeFunction> cprint = std::make_shared<LuaNativeFunction>([](VM& vm, int base_reg, int num_args) -> int {
        auto& st = vm.get_stack_mutable();
        *st[base_reg] = LuaValue(std::make_shared<LuaString>("hello"), LuaType::STRING);
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

    std::shared_ptr<LuaFunction> main_func = std::make_shared<LuaFunction>(bytecode, constants, std::vector<LuaValue>{}, std::vector<UpvalDesc>{_ENV}, std::vector<LocalVarinfo>{});
    std::shared_ptr<LuaClosure> main_closure = std::make_shared<LuaClosure>(main_func);
    vm = VM();
    vm.load(main_closure);
    vm.set_trace(true);
    vm.run();
    auto result = vm.get_stack_mutable()[0];
    assert(result.get()->getType() == LuaType::STRING);
    std::cout << "Result: " << result.get()->getObject()->toString() << std::endl;
}

void test_metamethod() {
    std::cout << "--- Testing Metamethod ---" << std::endl;
    UpvalDesc _ENV; _ENV.name = "_ENV"; _ENV.inStack = true; _ENV.idx = 0;

    std::shared_ptr<LuaNativeFunction> __add = std::make_shared<LuaNativeFunction>([](VM& vm, int base_reg, int num_args) -> int {
        auto& stack = vm.get_stack_mutable();
        *stack[base_reg] = LuaValue(std::make_shared<LuaInteger>(10), LuaType::NUMBER);
        return 1;
    });

    std::shared_ptr<LuaTable> a = std::make_shared<LuaTable>(); 
    std::shared_ptr<LuaTable> a_mt = std::make_shared<LuaTable>();
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

    std::shared_ptr<LuaFunction> main_func = std::make_shared<LuaFunction>(bytecode, constants, std::vector<LuaValue>{}, std::vector<UpvalDesc>{_ENV}, std::vector<LocalVarinfo>{});
    std::shared_ptr<LuaClosure> main_closure = std::make_shared<LuaClosure>(main_func);
    vm = VM();
    vm.load(main_closure);
    vm.set_trace(true);
    vm.run();
    auto result = vm.get_stack_mutable()[0];
    assert(result.get()->getType() == LuaType::NUMBER
        && std::dynamic_pointer_cast<LuaInteger>(result.get()->getObject())->getValue() == 10);
    std::cout << "Result: " << result.get()->getObject()->toString() << std::endl;
}

void test_stack_overflow() {
    std::cout << "--- Testing Stack Overflow Detection ---" << std::endl;
    UpvalDesc _ENV; _ENV.name = "_ENV"; _ENV.inStack = true; _ENV.idx = 0;
    UpvalDesc _ENV_f; _ENV_f.name = "_ENV"; _ENV_f.inStack = false; _ENV_f.idx = 0;

    std::vector<Instruction> bytecode_f = {
        CREATE_ABC(OpCode::GETTABUP, 10, 0, 0), // 1: R10 = _ENV["func"]
        CREATE_ABC(OpCode::MOVE, 11, 0, 0),     // 2: MOVE R11 = R0
        CREATE_ABC(OpCode::MOVE, 12, 1, 0),     // 3: MOVE R12 = R1
        CREATE_ABC(OpCode::MOVE, 13, 2, 0),     // 4: MOVE R13 = R2
        CREATE_ABC(OpCode::MOVE, 14, 3, 0),     // 5
        CREATE_ABC(OpCode::MOVE, 15, 4, 0),     // 6
        CREATE_ABC(OpCode::MOVE, 16, 5, 0),     // 7
        CREATE_ABC(OpCode::MOVE, 17, 6, 0),     // 8
        CREATE_ABC(OpCode::MOVE, 18, 7, 0),     // 9
        CREATE_ABC(OpCode::MOVE, 19, 8, 0),     // 10
        CREATE_ABC(OpCode::MOVE, 20, 9, 0),     // 11
        CREATE_ABC(OpCode::CALL, 10, 11, 1),    // 12: 再帰呼び出し
        CREATE_A(OpCode::RETURN0, 0)            // 13: RETURN0
    };

    
    std::vector<LuaValue> constants_f = {
        LuaValue(std::make_shared<LuaString>("func"), LuaType::STRING)
    };


    std::shared_ptr<LuaFunction> func = std::make_shared<LuaFunction>(bytecode_f, constants_f, std::vector<LuaValue>{}, std::vector<UpvalDesc>{_ENV_f}, std::vector<LocalVarinfo>{});

    std::vector<Instruction> bytecode = {
        CREATE_ABx(OpCode::CLOSURE, 1, 0),     // R(1) = closure(func)
        CREATE_ABC(OpCode::SETTABUP, 0, 0, 1), // _ENV["func"] = R(1)
        CREATE_ABC(OpCode::GETTABUP, 11, 0, 0), // R11 = _ENV["func"]
        CREATE_ABC(OpCode::LOADI, 1, 1, 0),    // 5: R1 = 1
        CREATE_ABC(OpCode::LOADI, 2, 2, 0),    // 6: R2 = 2
        CREATE_ABC(OpCode::LOADI, 3, 3, 0),    // 7: R3 = 3
        CREATE_ABC(OpCode::LOADI, 4, 4, 0),    // 8: R4 = 4
        CREATE_ABC(OpCode::LOADI, 5, 5, 0),    // 9: R5 = 5
        CREATE_ABC(OpCode::LOADI, 6, 6, 0),    // 10: R6 = 6
        CREATE_ABC(OpCode::LOADI, 7, 7, 0),    // 11: R7 = 7
        CREATE_ABC(OpCode::LOADI, 8, 8, 0),    // 12: R8 = 8
        CREATE_ABC(OpCode::LOADI, 9, 9, 0),    // 13: R9 = 9
        CREATE_ABC(OpCode::LOADI, 10, 10, 0),  // 14: R10 = 10
        CREATE_ABC(OpCode::CALL, 11, 11, 1),    // 15: CALL R(11) with 10 args
        CREATE_A(OpCode::RETURN1, 0)           // 16: RETURN
    };

    std::vector<LuaValue> constants = {
        LuaValue(std::make_shared<LuaString>("func"), LuaType::STRING)
    };

    std::vector<LuaValue> protos = {
        LuaValue(func, LuaType::FUNCTION)
    };

    std::shared_ptr<LuaFunction> main_func = std::make_shared<LuaFunction>(bytecode,
        constants,
        protos,
        std::vector<UpvalDesc>{_ENV},
        std::vector<LocalVarinfo>{
            LocalVarinfo("a", 1, 14),
            LocalVarinfo("b", 1, 14),
            LocalVarinfo("c", 1, 14),
            LocalVarinfo("d", 1, 14),
            LocalVarinfo("e", 1, 14),
            LocalVarinfo("f", 1, 14),
            LocalVarinfo("g", 1, 14),
            LocalVarinfo("h", 1, 14),
            LocalVarinfo("i", 1, 14),
            LocalVarinfo("j", 1, 14),
        }
    );
    std::shared_ptr<LuaClosure> main_closure = std::make_shared<LuaClosure>(main_func);
    vm = VM();
    vm.load(main_closure);
    vm.set_trace(false);
    vm.run();
}

void test_vm_dump() {
    std::cout << "--- Testing VM Error Dumping ---" << std::endl;
    UpvalDesc _ENV; _ENV.name = "_ENV"; _ENV.inStack = true; _ENV.idx = 0;

    std::vector<Instruction> bytecode = {
        CREATE_ABC(255, 255, 255, 255)
    };

    std::shared_ptr<LuaFunction> main_func = std::make_shared<LuaFunction>(bytecode,
        std::vector<LuaValue>{},
        std::vector<LuaValue>{},
        std::vector<UpvalDesc>{_ENV},
        std::vector<LocalVarinfo>{}
    );
    std::shared_ptr<LuaClosure> main_closure = std::make_shared<LuaClosure>(main_func);
    vm = VM();
    vm.load(main_closure);
    vm.set_trace(true);
    vm.run();
}

void test_baselib() {
    std::cout << "--- Testing Basic Library ---" << std::endl;
    UpvalDesc _ENV; _ENV.name = "_ENV"; _ENV.inStack = true; _ENV.idx = 0;

    std::vector<Instruction> bytecode = {
        CREATE_A(OpCode::VARARGPREP, 0),
        /*
        CREATE_ABC(OpCode::GETTABUP, 0, 0, 0),
        CREATE_ABx(OpCode::LOADK, 1, 1),
        CREATE_ABC(OpCode::CALL, 0, 2, 1),
        */
        CREATE_ABC(OpCode::GETTABUP, 0, 0, 0), // R0 = _ENV["print"]
        CREATE_ABC(OpCode::GETTABUP, 1, 0, 1), // R1 = _ENV["_VERSION"]
        CREATE_ABC(OpCode::CALL, 0, 2, 1), // R0(R1)
        CREATE_ABC(OpCode::RETURN, 0, 1, 1) // (EOF)
    };

    std::vector<LuaValue> constants = {
        LuaValue(std::make_shared<LuaString>("print"), LuaType::STRING),
        LuaValue(std::make_shared<LuaString>("_VERSION"), LuaType::STRING)
    };

    std::shared_ptr<LuaFunction> main_func = std::make_shared<LuaFunction>(bytecode,
        constants,
        std::vector<LuaValue>{},
        std::vector<UpvalDesc>{_ENV},
        std::vector<LocalVarinfo>{}
    );
    std::shared_ptr<LuaClosure> main_closure = std::make_shared<LuaClosure>(main_func);
    vm = VM();
    vm.load(main_closure);
    vm.set_trace(true);
    vm.run();
    //dump_critical_error(vm, "test");
}

int main(int argc, char **argv)
{
    try {
        std::cout << "Starting tests..." << std::endl;
        test_cfunction_call();
        std::cout << "CFunction test passed." << std::endl;
        test_metamethod();
        std::cout << "Metamethod test passed." << std::endl;
        //test_stack_overflow();
        //test_vm_dump();
        test_baselib();
        
        std::cout << "All tests passed." << std::endl;
    } catch (const std::runtime_error& e) {
        dump_critical_error(vm, e.what());
    } catch (const LuaError& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}