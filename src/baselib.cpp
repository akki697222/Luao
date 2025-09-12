#include <map>
#include <object.hpp>
#include <function.hpp>
#include <table.hpp>
#include <vm.hpp>
#include <libs.hpp>

using namespace luao;

static int baselib_print(VM& vm, int base_reg, int num_args) {
    for (int i = 0; i < num_args; ++i) {
        const auto& val = *vm.get_stack()[base_reg + i];
        std::cout << val.toString() << "\t";
    }
    std::cout << std::endl;
    return 0;
}

static int baselib_assert(VM& vm, int base_reg, int num_args) {
    auto& stack = vm.get_stack_mutable();

    if (num_args < 1) {
        throw LuaError("bad argument #1 to 'assert' (value expected)");
    }

    const auto& expr = *stack[base_reg];
    std::string msg = "assertion failed!";

    if (num_args > 1) {
        const auto& msg_v = *stack[base_reg + 1];
        if (auto msg_v_ptr = std::dynamic_pointer_cast<const LuaString>(msg_v.getObject())) {
            msg = msg_v_ptr->getValue();
        } else {
            msg = "(error object is a " + msg_v.typeName() + " value)";
        }
    }

    if (!vm.as_bool(expr)) {
        throw LuaError(msg);
    }

    return num_args;
}

std::map<LuaString, LuaValue> getbaselib() {
    std::map<LuaString, LuaValue> lib;
    lib[LuaString("print")] = LuaValue(
        std::make_shared<LuaNativeFunction>(baselib_print),
        LuaType::FUNCTION
    );
    lib[LuaString("assert")] = LuaValue(
        std::make_shared<LuaNativeFunction>(baselib_assert),
        LuaType::FUNCTION
    );
    return lib;
}