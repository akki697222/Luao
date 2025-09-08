#pragma once

#include <object.hpp>
#include <luao.hpp>
#include <string>

class LuaTable : public LuaGCObject {
public:
    LuaTable() = default;
    ~LuaTable() override = default;

    LuaType getType() const override { return LuaType::TABLE; }
    std::string typeName() const override { return "table"; }
private:

};