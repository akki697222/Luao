#pragma once

#include "opcodes.hpp"
#include "function.hpp"
#include <string>
#include <vector>

namespace luao {

std::string disassemble_instruction(Instruction i, const LuaFunction* func);

} // namespace luao
