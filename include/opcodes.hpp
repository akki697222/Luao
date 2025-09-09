#pragma once

#include <cstdint>
#include <string_view>

namespace luao {

    using Instruction = uint32_t;

    enum class OpMode {
        iABC,
        iABx,
        iAsBx,
        iAx,
        isJ
    };

    enum class OpCode {
        /*----------------------------------------------------------------------
        name          args    description
        ------------------------------------------------------------------------*/
        MOVE,         /* A B     R[A] := R[B]                                    */
        LOADI,        /* A sBx   R[A] := sBx                                     */
        LOADF,        /* A sBx   R[A] := (lua_Number)sBx                         */
        LOADK,        /* A Bx    R[A] := K[Bx]                                   */
        LOADKX,       /* A       R[A] := K[extra arg]                            */
        LOADFALSE,    /* A       R[A] := false                                   */
        LFALSESKIP,   /* A       R[A] := false; pc++     (*)                     */
        LOADTRUE,     /* A       R[A] := true                                    */
        LOADNIL,      /* A B     R[A], R[A+1], ..., R[A+B] := nil                */
        GETUPVAL,     /* A B     R[A] := UpValue[B]                              */
        SETUPVAL,     /* A B     UpValue[B] := R[A]                              */
        GETGLOBAL,    /* A Bx    R[A] := G[K[Bx]]                                */
        SETGLOBAL,    /* A Bx    G[K[Bx]] := R[A]                                */
        GETTABUP,     /* A B C   R[A] := UpValue[B][K[C]:shortstring]            */
        GETTABLE,     /* A B C   R[A] := R[B][R[C]]                              */
        GETI,         /* A B C   R[A] := R[B][C]                                 */
        GETFIELD,     /* A B C   R[A] := R[B][K[C]:shortstring]                  */
        SETTABUP,     /* A B C   UpValue[A][K[B]:shortstring] := RK(C)           */
        SETTABLE,     /* A B C   R[A][R[B]] := RK(C)                             */
        SETI,         /* A B C   R[A][B] := RK(C)                                */
        SETFIELD,     /* A B C   R[A][K[B]:shortstring] := RK(C)                 */
        NEWTABLE,     /* A B C k R[A] := {}                                      */
        SELF,         /* A B C   R[A+1] := R[B]; R[A] := R[B][RK(C):string]      */
        ADDI,         /* A B sC  R[A] := R[B] + sC                               */
        ADDK,         /* A B C   R[A] := R[B] + K[C]:number                      */
        SUBK,         /* A B C   R[A] := R[B] - K[C]:number                      */
        MULK,         /* A B C   R[A] := R[B] * K[C]:number                      */
        MODK,         /* A B C   R[A] := R[B] % K[C]:number                      */
        POWK,         /* A B C   R[A] := R[B] ^ K[C]:number                      */
        DIVK,         /* A B C   R[A] := R[B] / K[C]:number                      */
        IDIVK,        /* A B C   R[A] := R[B] // K[C]:number                     */
        BANDK,        /* A B C   R[A] := R[B] & K[C]:integer                     */
        BORK,         /* A B C   R[A] := R[B] | K[C]:integer                     */
        BXORK,        /* A B C   R[A] := R[B] ~ K[C]:integer                     */
        SHRI,         /* A B sC  R[A] := R[B] >> sC                              */
        SHLI,         /* A B sC  R[A] := sC << R[B]                              */
        ADD,          /* A B C   R[A] := R[B] + R[C]                             */
        SUB,          /* A B C   R[A] := R[B] - R[C]                             */
        MUL,          /* A B C   R[A] := R[B] * R[C]                             */
        MOD,          /* A B C   R[A] := R[B] % R[C]                             */
        POW,          /* A B C   R[A] := R[B] ^ R[C]                             */
        DIV,          /* A B C   R[A] := R[B] / R[C]                             */
        IDIV,         /* A B C   R[A] := R[B] // R[C]                            */
        BAND,         /* A B C   R[A] := R[B] & R[C]                             */
        BOR,          /* A B C   R[A] := R[B] | R[C]                             */
        BXOR,         /* A B C   R[A] := R[B] ~ R[C]                             */
        SHL,          /* A B C   R[A] := R[B] << R[C]                            */
        SHR,          /* A B C   R[A] := R[B] >> R[C]                            */
        MMBIN,        /* A B C   call C metamethod over R[A] and R[B]    (*)     */
        MMBINI,       /* A sB C k        call C metamethod over R[A] and sB      */
        MMBINK,       /* A B C k         call C metamethod over R[A] and K[B]    */
        UNM,          /* A B     R[A] := -R[B]                                   */
        BNOT,         /* A B     R[A] := ~R[B]                                   */
        NOT,          /* A B     R[A] := not R[B]                                */
        LEN,          /* A B     R[A] := #R[B] (length operator)                 */
        CONCAT,       /* A B     R[A] := R[A].. ... ..R[A + B - 1]               */
        CLOSE,        /* A       close all upvalues >= R[A]                      */
        TBC,          /* A       mark variable A "to be closed"                  */
        JMP,          /* sJ      pc += sJ                                        */
        EQ,           /* A B k   if ((R[A] == R[B]) ~= k) then pc++              */
        LT,           /* A B k   if ((R[A] <  R[B]) ~= k) then pc++              */
        LE,           /* A B k   if ((R[A] <= R[B]) ~= k) then pc++              */
        EQK,          /* A B k   if ((R[A] == K[B]) ~= k) then pc++              */
        EQI,          /* A sB k  if ((R[A] == sB) ~= k) then pc++                */
        LTI,          /* A sB k  if ((R[A] < sB) ~= k) then pc++                 */
        LEI,          /* A sB k  if ((R[A] <= sB) ~= k) then pc++                */
        GTI,          /* A sB k  if ((R[A] > sB) ~= k) then pc++                 */
        GEI,          /* A sB k  if ((R[A] >= sB) ~= k) then pc++                */
        TEST,         /* A k     if (not R[A] == k) then pc++                    */
        TESTSET,      /* A B k   if (not R[B] == k) then pc++ else R[A] := R[B] (*) */
        CALL,         /* A B C   R[A], ... ,R[A+C-2] := R[A](R[A+1], ... ,R[A+B-1]) */
        TAILCALL,     /* A B C k return R[A](R[A+1], ... ,R[A+B-1])              */
        RETURN,       /* A B C k return R[A], ... ,R[A+B-2]      (see note)      */
        RETURN0,      /*           return                                          */
        RETURN1,      /* A       return R[A]                                     */
        FORLOOP,      /* A Bx    update counters; if loop continues then pc-=Bx; */
        FORPREP,      /* A Bx    <check values and prepare counters>;
                                if not to run then pc+=Bx+1;                    */
        TFORPREP,     /* A Bx    create upvalue for R[A + 3]; pc+=Bx             */
        TFORCALL,     /* A C     R[A+4], ... ,R[A+3+C] := R[A](R[A+1], R[A+2]);  */
        TFORLOOP,     /* A Bx    if R[A+2] ~= nil then { R[A]=R[A+2]; pc -= Bx } */
        SETLIST,      /* A B C k R[A][C+i] := R[A+i], 1 <= i <= B                */
        CLOSURE,      /* A Bx    R[A] := closure(KPROTO[Bx])                     */
        VARARG,       /* A C     R[A], R[A+1], ..., R[A+C-2] = vararg            */
        VARARGPREP,   /* A       (adjust vararg parameters)                      */
        EXTRAARG      /* Ax      extra (larger) argument for previous opcode     */
    };

    std::string_view to_string(OpCode op);

} // namespace luao
