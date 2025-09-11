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

struct LocalVarinfo {
    std::string name;
    int startpc;
    int endpc;

    LocalVarinfo() : name(""), startpc(0), endpc(0) {}

    // 引数付きコンストラクタ
    LocalVarinfo(const std::string& name, int start, int end)
        : name(name), startpc(start), endpc(end) {}
};

class LuaFunction : public LuaGCObject {
public:
    LuaFunction(
        std::vector<Instruction> bytecode, 
        std::vector<LuaValue> constants, 
        std::vector<LuaValue> protos, 
        std::vector<UpvalDesc> upvalDescs,
        std::vector<LocalVarinfo> localvars
    )
        : bytecode(std::move(bytecode)),
          constants(std::move(constants)), 
          protos(std::move(protos)),
          upvalDescs(std::move(upvalDescs)),
          localvars(std::move(localvars)) 
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
        std::vector<LocalVarinfo> localvars,
        std::string source,
        std::vector<Lineinfo> lineinfos,
        int linedefined,
        int lastlinedefined
    )
        : bytecode(std::move(bytecode)),
          constants(std::move(constants)), 
          protos(std::move(protos)),
          upvalDescs(std::move(upvalDescs)),
          localvars(std::move(localvars)),
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
    std::string getSource() const { return source; }
    const std::vector<Lineinfo>& getLineinfos() const { return lineinfos; }
    const int& getLinedefined() const { return linedefined; }
    const int& getLastlinedefined() const { return lastlinedefined; }
    
    void setVarargs(const std::vector<LuaValue>& args) { varargs = args; }

private:
    std::vector<Instruction> bytecode;
    std::vector<LuaValue> constants;
    std::vector<LuaValue> protos;
    std::vector<UpvalDesc> upvalDescs;
    std::vector<LuaValue> varargs;
    std::vector<LocalVarinfo> localvars;
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
