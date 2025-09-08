#pragma once

#include <luao.hpp>
#include <string>
#include <vector>
#include <sstream>

class LuaValue {
public:
    LuaValue(LuaObject* obj, LuaType type) : obj(obj), type(type) {}
    LuaType getType() const { return type; }
    LuaObject* getObject() const { return obj; }
private:
    LuaObject* obj;
    LuaType type;
};

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

class LuaGCObject : public LuaObject {
public:
    LuaGCObject() : refCount(0) {}
    virtual ~LuaGCObject() = default;

    void retain() { ++refCount; }
    void release() { if (--refCount == 0) delete this; }
private:
    int refCount;
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