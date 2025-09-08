#pragma once

#include <luao.hpp>
#include <string>
#include <vector>
#include <sstream>

namespace luao {

class LuaObject;
class LuaGCObject;
class LuaValue;
class LuaInteger;
class LuaNumber;
class LuaString;

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

class LuaGCObject : public LuaObject {
public:
    LuaGCObject() : refCount(0) {}
    virtual ~LuaGCObject() = default;

    void retain() { ++refCount; }
    void release() { if (--refCount == 0) delete this; }
private:
    int refCount;
};

class LuaInteger : public LuaGCObject {
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

class LuaNumber : public LuaGCObject {
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

class LuaString : public LuaGCObject {
public:
    LuaString(const std::string& value) : value(value) {}
    const std::string& getValue() const { return value; }
    void setValue(const std::string& v) { value = v; }
    LuaType getType() const override { return LuaType::STRING; }
    std::string toString() const override { return value; }
    std::string typeName() const override { return "string"; }
private:
    std::string value;
};

class LuaValue {
public:
    LuaValue() : obj(nullptr), type(LuaType::NIL) {}

    LuaValue(LuaObject* obj, LuaType type) : obj(obj), type(type) {
        if (isGCObject() && obj != nullptr) {
            static_cast<LuaGCObject*>(obj)->retain();
        }
    }

    LuaValue(const LuaValue& other) : obj(other.obj), type(other.type) {
        if (isGCObject() && obj != nullptr) {
            static_cast<LuaGCObject*>(obj)->retain();
        }
    }

    LuaValue& operator=(const LuaValue& other) {
        if (this != &other) {
            if (isGCObject() && obj != nullptr) {
                static_cast<LuaGCObject*>(obj)->release();
            }

            obj = other.obj;
            type = other.type;

            if (isGCObject() && obj != nullptr) {
                static_cast<LuaGCObject*>(obj)->retain();
            }
        }
        return *this;
    }

    ~LuaValue() {
        if (isGCObject() && obj != nullptr) {
            static_cast<LuaGCObject*>(obj)->release();
        }
    }

    LuaType getType() const { return type; }
    LuaObject* getObject() const { return obj; }

    bool isGCObject() const {
        switch (type) {
            case LuaType::NUMBER:
            case LuaType::STRING:
            case LuaType::TABLE:
            case LuaType::FUNCTION:
            case LuaType::USERDATA:
            case LuaType::THREAD:
            case LuaType::OBJECT:
            case LuaType::INSTANCE:
            case LuaType::THROWABLE:
                return true;
            default:
                return false;
        }
    }
private:
    LuaObject* obj;
    LuaType type;
};

} // namespace luao