#pragma once

#include <object.hpp>
#include <luao.hpp>
#include <vector>
#include "opcodes.hpp"

namespace luao {

class LuaFunction : public LuaGCObject {
public:
    LuaFunction(std::vector<Instruction> bytecode, std::vector<LuaValue> constants)
        : bytecode(std::move(bytecode)), constants(std::move(constants)) {}

    LuaType getType() const override { return LuaType::FUNCTION; }
    std::string typeName() const override { return "function"; }

    const std::vector<Instruction>& getBytecode() const { return bytecode; }
    const std::vector<LuaValue>& getConstants() const { return constants; }

private:
    std::vector<Instruction> bytecode;
    std::vector<LuaValue> constants;
};

} // namespace luao
