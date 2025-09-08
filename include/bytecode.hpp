/*
    Luao Bytecode generator
*/

#pragma once

#include <cstdint>
#include <cstddef>
#include <luao.hpp>

enum class OpCodes {

};

class BytecodeDecoder {
public:
    BytecodeDecoder(const uint8_t* bytecode, size_t size);
};