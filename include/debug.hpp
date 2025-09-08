#pragma once

#include "opcodes.hpp"
#include <string>

namespace luao {

std::string disassemble_instruction(Instruction i);

} // namespace luao
