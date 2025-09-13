#pragma once

#include <object.hpp>
#include <memory>
#include <list>

namespace luao {

class VM;

class UpValue : public LuaObject {
public:
    UpValue(VM* vm, std::shared_ptr<LuaValue> location)
        : vm(vm), location_(location), open_(true) {}

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

    void close();

    LuaType getType() const override { return LuaType::USERDATA; }
    std::string typeName() const override { return "upvalue"; }

    std::list<std::shared_ptr<UpValue>>::iterator getIterator() {
        return open_upval_iter;
    }

    void setIterator(std::list<std::shared_ptr<UpValue>>::iterator it) {
        open_upval_iter = it;
    }

private:
    VM* vm;
    std::weak_ptr<LuaValue> location_;
    LuaValue closed_;
    bool open_;
    std::list<std::shared_ptr<UpValue>>::iterator open_upval_iter;
};

} // namespace luao
