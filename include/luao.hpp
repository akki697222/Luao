#pragma once

#include <iostream>

/* Infomation */
#define LUAO_VERSION "Luao 1.0"
#define LUAO_COPYRIGHT "Copyright (C) 2025 akki697222"
#define LUAO_LICENSE "MIT License"

/* Bytecode */
#define LUAO_BMAGIC 0x1B4C7561 /* "\x1bLua" */
#define LUAO_BVERSION 0x54   /* Lua 5.4 */

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
#define LUAO_PROTO          9
/* Luao Extensions */
#define LUAO_TOBJECT		10  /* Object/Class */
#define LUAO_TINSTANCE      11 /* Class Instance */
#define LUAO_TTYPE          12 /* Type Object */
#define LUAO_TTHROWABLE     13 /* Throwable */

#define LUAO_NUMTYPES		14

#define LUAERR(msg) std::cerr << msg << std::endl

/* General */
#define LUAO_GLOBAL "_G"
#define LUAO_ENV "_ENV"

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
    PROTO = LUAO_PROTO,
    OBJECT = LUAO_TOBJECT,         /* Object/Class */
    INSTANCE = LUAO_TINSTANCE,     /* Class Instance */
    TYPE = LUAO_TTYPE,             /* Type Object */
    THROWABLE = LUAO_TTHROWABLE,   /* Throwable */
};

namespace luao {

}