#pragma once

#include <object.hpp>
#include <function.hpp>
#include <upvalue.hpp>
#include <vector>

namespace luao {

class LuaClosure : public LuaGCObject {
public:
    explicit LuaClosure(std::shared_ptr<LuaFunction> function) : function_(function) {}

    ~LuaClosure() = default;

    std::shared_ptr<LuaFunction> getFunction() const {
        return function_;
    }

    std::vector<std::shared_ptr<UpValue>>& getUpvalues() {
        return upvalues_;
    }

    void setUpvalue(int index, std::shared_ptr<UpValue> upvalue) {
        upvalues_[index] = upvalue;
    }

    LuaType getType() const override { return LuaType::FUNCTION; }
    std::string typeName() const override { return "function"; }

private:
    std::shared_ptr<LuaFunction> function_;
    std::vector<std::shared_ptr<UpValue>> upvalues_;
};

} // namespace luao
