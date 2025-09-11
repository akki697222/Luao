#pragma once

#include <object.hpp>

namespace luao {

class UpValue : public LuaGCObject {
public:
    explicit UpValue(LuaValue* location) : location_(location) {}

    LuaValue* getLocation() const {
        return location_;
    }

    LuaValue getValue() const {
        return *location_;
    }

    void setValue(const LuaValue& value) {
        *location_ = value;
    }

    void close() {
        closed_ = *location_;
        location_ = &closed_;
    }
    
    LuaType getType() const override { return LuaType::USERDATA; }
    std::string typeName() const override { return "upvalue"; }

private:
    LuaValue* location_;
    LuaValue closed_;
};

} // namespace luao
