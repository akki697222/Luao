#pragma once

#include "object.hpp"
#include "function.hpp"
#include "upvalue.hpp"
#include <vector>

namespace luao {

class LuaClosure : public LuaGCObject {
public:
    explicit LuaClosure(LuaFunction* function) : function_(function) {
        if (function_) {
            function_->retain();
        }
    }

    ~LuaClosure() override {
        if (function_) {
            function_->release();
        }
        for (auto* upvalue : upvalues_) {
            upvalue->release();
        }
    }

    LuaFunction* getFunction() const {
        return function_;
    }

    std::vector<UpValue*>& getUpvalues() {
        return upvalues_;
    }

    LuaType getType() const override { return LuaType::FUNCTION; }
    std::string typeName() const override { return "function"; }

private:
    LuaFunction* function_;
    std::vector<UpValue*> upvalues_;
};

} // namespace luao
