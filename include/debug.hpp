#pragma once

#include <opcodes.hpp>
#include <function.hpp>
#include <string>
#include <vector>

namespace luao {

std::string disassemble_instruction(Instruction i, const std::shared_ptr<LuaFunction> func);

} // namespace luao
