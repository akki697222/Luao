#pragma once

#include <luao.hpp>
#include <string>
#include <vector>
#include <sstream>
#include <memory>

namespace luao {

class LuaGCObject;
class LuaValue;
class LuaInteger;
class LuaNumber;
class LuaString;
class LuaFunction;
class LuaTable;
class LuaClosure;

class LuaObject {
public:
    virtual ~LuaObject() = default;
    virtual LuaType getType() const = 0;
    virtual std::string typeName() const = 0;

    virtual std::string toString() const {
        std::ostringstream oss;
        oss << typeName() << ": " << this;
        return oss.str();
    }
};

class LuaGCObject : public LuaObject, public std::enable_shared_from_this<LuaGCObject> {
public:
    LuaGCObject() = default;
    virtual ~LuaGCObject() = default;

    // metatables
    std::shared_ptr<class LuaTable> getMetatable() const { return metatable; }
    void setMetatable(std::shared_ptr<class LuaTable> mt) { metatable = mt; }

    LuaValue getMetamethod(const LuaValue& key) const;

private:
    std::shared_ptr<class LuaTable> metatable = nullptr;
};

/* immediate values (no GC) */
class LuaInteger : public LuaObject {
public:
    LuaInteger(luaInt value) : value(value) {}
    luaInt getValue() const { return value; }
    void setValue(luaInt v) { value = v; }
    LuaType getType() const override { return LuaType::NUMBER; }
    std::string toString() const override { return std::to_string(value); }
    std::string typeName() const override { return "number"; }
private:
    luaInt value;
};

class LuaNumber : public LuaObject {
public:
    LuaNumber(luaNumber value) : value(value) {}
    luaNumber getValue() const { return value; }
    void setValue(luaNumber v) { value = v; }
    LuaType getType() const override { return LuaType::NUMBER; }
    std::string toString() const override { return std::to_string(value); }
    std::string typeName() const override { return "number"; }
private:
    luaNumber value;
};

class LuaBool : public LuaObject {
public:
    LuaBool(bool value) : value(value) {}
    bool getValue() const { return value; }
    void setValue(bool v) { value = v; }
    LuaType getType() const override { return LuaType::BOOLEAN; }
    std::string toString() const override { return value ? "true" : "false"; }
    std::string typeName() const override { return "boolean"; }
private:
    bool value;
};

class LuaString : public LuaGCObject {
public:
    LuaString(const std::string& value) : value(value) {}
    const std::string& getValue() const { return value; }
    void setValue(const std::string& v) { value = v; }
    LuaType getType() const override { return LuaType::STRING; }
    std::string toString() const override { return value; }
    std::string typeName() const override { return "string"; }

    bool operator<(const LuaString& other) const noexcept {
        return value < other.value;
    }

    bool operator==(const LuaString& other) const noexcept {
        return value == other.value;
    }
private:
    std::string value;
};

/* LuaValue using shared_ptr for GC objects */
class LuaValue {
public:
    LuaValue() : type(LuaType::NIL) {}

    LuaValue(std::shared_ptr<LuaObject> obj, LuaType type) : obj(obj), type(type) {}

    LuaValue(const std::shared_ptr<LuaValue>& other) : obj(other.get()->getObject()), type(other.get()->getType()) {}

    LuaValue& operator=(const LuaValue& other) {
        if (this != &other) {
            obj = other.obj;
            type = other.type;
        }
        return *this;
    }

    LuaType getType() const { return type; }
    std::shared_ptr<LuaObject> getObject() const { return obj; }

    std::string typeName() const {
        return obj ? obj->typeName() : "nil";
    }

    std::string toString() const {
        return obj ? obj->toString() : "nil";
    }

    bool isGCObject() const {
        switch (type) {
            case LuaType::STRING:
            case LuaType::TABLE:
            case LuaType::FUNCTION:
            case LuaType::USERDATA:
            case LuaType::THREAD:
            case LuaType::OBJECT:
            case LuaType::INSTANCE:
            case LuaType::THROWABLE:
            case LuaType::PROTO:
                return true;
            default:
                return false;
        }
    }

private:
    std::shared_ptr<LuaObject> obj;
    LuaType type;
};

} // namespace luao
