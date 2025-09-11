#pragma once

#include <object.hpp>
#include <memory>

namespace luao {

class UpValue : public LuaObject {
public:
    UpValue(std::shared_ptr<LuaValue> location)
        : location_(location), open_(true) {}

    bool isOpen() const { return open_; }

    std::weak_ptr<LuaValue> getLocation() {
        return location_;
    }

    LuaValue getValue() const {
        if (auto loc = location_.lock()) return *loc;
        return closed_; 
    }

    void setValue(const LuaValue& value) {
        if (auto loc = location_.lock()) *loc = value;
        else closed_ = value;
    }

    void close() {
        if (!open_) return;
        if (auto loc = location_.lock()) closed_ = *loc;
        location_.reset();
        open_ = false;
    }

    LuaType getType() const override { return LuaType::USERDATA; }
    std::string typeName() const override { return "upvalue"; }

private:
    std::weak_ptr<LuaValue> location_;
    LuaValue closed_;
    bool open_;
};

} // namespace luao
