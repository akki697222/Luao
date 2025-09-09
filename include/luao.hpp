#pragma once

#include <vm.hpp>
#include <table.hpp>
#include <object.hpp>
#include <iostream>

/* Infomation */
#define LUAO_VERSION "Luao 1.0"
#define LUAO_COPYRIGHT "Copyright (C) 2025 akki697222"
#define LUAO_LICENSE "MIT License"

/* Bytecode */
#define LUAO_BMAGIC 0x1C4C7561 /* "\x1cLua" */
#define LUAO_BVERSION 0x10   /* Luao version 1.0 (Implements Lua 5.4) */

/* Default Lua Typedefs */
#define LUAO_TNIL		    0
#define LUAO_TBOOLEAN		1
#define LUAO_TLIGHTUSERDATA	2
#define LUAO_TNUMBER		3
#define LUAO_TSTRING		4
#define LUAO_TTABLE		    5
#define LUAO_TFUNCTION		6
#define LUAO_TUSERDATA		7
#define LUAO_TTHREAD		8
/* Luao Extensions */
#define LUAO_TOBJECT		9  /* Object/Class */
#define LUAO_TINSTANCE      10 /* Class Instance */
#define LUAO_TTYPE          11 /* Type Object */
#define LUAO_TTHROWABLE     12 /* Throwable */

#define LUAO_NUMTYPES		13

#define LUAERR(msg) std::cerr << msg << std::endl

typedef long long luaInt;
typedef double luaNumber;

enum class LuaType {
    NIL = LUAO_TNIL,
    BOOLEAN = LUAO_TBOOLEAN,
    LIGHTUSERDATA = LUAO_TLIGHTUSERDATA,
    NUMBER = LUAO_TNUMBER,
    STRING = LUAO_TSTRING,
    TABLE = LUAO_TTABLE,
    FUNCTION = LUAO_TFUNCTION,
    USERDATA = LUAO_TUSERDATA,
    THREAD = LUAO_TTHREAD,
    OBJECT = LUAO_TOBJECT,         /* Object/Class */
    INSTANCE = LUAO_TINSTANCE,     /* Class Instance */
    TYPE = LUAO_TTYPE,             /* Type Object */
    THROWABLE = LUAO_TTHROWABLE    /* Throwable */
};

namespace luao {
    LuaValue* get_metamethod(const LuaTable& mt, LuaValue& index) {
        LuaValue mm = mt.get(index);

    }
}