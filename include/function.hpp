#pragma once

#include <object.hpp>
#include <luao.hpp>
#include <vector>
#include <functional>
#include <opcodes.hpp>

namespace luao {

class VM; // forward declaration for native function callback

struct UpvalDesc {
    std::string name;
    bool inStack;    
    int idx;         
};

struct Lineinfo {
    int op;
    int line;
};

class LuaFunction : public LuaGCObject {
public:
    LuaFunction(
        std::vector<Instruction> bytecode, 
        std::vector<LuaValue> constants, 
        std::vector<LuaValue> protos, 
        std::vector<UpvalDesc> upvalDescs
    )
        : bytecode(std::move(bytecode)),
          constants(std::move(constants)), 
          protos(std::move(protos)),
          upvalDescs(std::move(upvalDescs)) 
    {
        source = "<none>";
        lineinfos = {};
        linedefined = 0;
        lastlinedefined = 0;
    }

    LuaFunction(
        std::vector<Instruction> bytecode, 
        std::vector<LuaValue> constants, 
        std::vector<LuaValue> protos, 
        std::vector<UpvalDesc> upvalDescs,
        std::string source,
        std::vector<Lineinfo> lineinfos,
        int linedefined,
        int lastlinedefined
    )
        : bytecode(std::move(bytecode)),
          constants(std::move(constants)), 
          protos(std::move(protos)),
          upvalDescs(std::move(upvalDescs)),
          source(std::move(source)),
          lineinfos(std::move(lineinfos)),
          linedefined(linedefined),
          lastlinedefined(lastlinedefined)
    {
        
    }


    LuaType getType() const override { return LuaType::FUNCTION; }
    std::string typeName() const override { return "prototype"; }

    const std::vector<Instruction>& getBytecode() const { return bytecode; }
    const std::vector<LuaValue>& getConstants() const { return constants; }
    const std::vector<LuaValue>& getProtos() const { return protos; }
    const std::vector<UpvalDesc>& getUpvalDescs() const { return upvalDescs; }
    const std::vector<LuaValue>& getVarargs() const { return varargs; }
    
    void setVarargs(const std::vector<LuaValue>& args) { varargs = args; }

private:
    std::vector<Instruction> bytecode;
    std::vector<LuaValue> constants;
    std::vector<LuaValue> protos;
    std::vector<UpvalDesc> upvalDescs;
    std::vector<LuaValue> varargs;
    std::string source;
    std::vector<Lineinfo> lineinfos;
    int linedefined;
    int lastlinedefined;
};

} // namespace luao

namespace luao {

class LuaNativeFunction : public LuaGCObject {
public:
    using CFunc = std::function<int(VM& vm, int base_reg, int num_args)>;

    explicit LuaNativeFunction(CFunc fn) : fn_(std::move(fn)) {}

    int call(VM& vm, int base_reg, int num_args) {
        return fn_(vm, base_reg, num_args);
    }

    LuaType getType() const override { return LuaType::FUNCTION; }
    std::string typeName() const override { return "cfunction"; }

private:
    CFunc fn_;
};

} // namespace luao
