#pragma once

#include <object.hpp>
#include <memory>

namespace luao {

class UpValue : public LuaObject {
public:
    explicit UpValue(std::shared_ptr<LuaValue> location) : location_(location) {}

    std::shared_ptr<LuaValue> getLocation() const {
        return location_;
    }

    LuaValue getValue() const {
        return *location_;
    }

    void setValue(const LuaValue& value) {
        *location_ = value;
    }

    void close() {
        closed_ = *location_.get();
        location_ = std::make_shared<LuaValue>(closed_);
    }
    
    LuaType getType() const override { return LuaType::USERDATA; }
    std::string typeName() const override { return "upvalue"; }

private:
    std::shared_ptr<LuaValue> location_;
    LuaValue closed_;
};

} // namespace luao
