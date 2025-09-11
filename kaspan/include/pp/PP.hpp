#pragma once

// clang-format off

// === STRINGIFY ===
#define STRINGIFY(X) #X

// === TOSTRING ===
#define TOSTRING(X) STRINGIFY(X)

// === CAT ===
#define CAT_IMPL(A,B) A##B
#define CAT(A,B) CAT_IMPL(A,B)

// === ARGS ===
#define ARGS_0()
#define ARGS_1(_1) _1
#define ARGS_2(_1,_2) _1,_2
#define ARGS_3(_1,_2,_3) _1,_2,_3
#define ARGS_4(_1,_2,_3,_4) _1,_2,_3,_4
#define ARGS_5(_1,_2,_3,_4,_5) _1,_2,_3,_4,_5
#define ARGS_6(_1,_2,_3,_4,_5,_6) _1,_2,_3,_4,_5,_6
#define ARGS_7(_1,_2,_3,_4,_5,_6,_7) _1,_2,_3,_4,_5,_6,_7
#define ARGS_8(_1,_2,_3,_4,_5,_6,_7,_8) _1,_2,_3,_4,_5,_6,_7,_8
#define ARGS_9(_1,_2,_3,_4,_5,_6,_7,_8,_9) _1,_2,_3,_4,_5,_6,_7,_8,_9
#define ARGS_10(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10) _1,_2,_3,_4,_5,_6,_7,_8,_9,_10
#define ARGS_11(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11) _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11
#define ARGS_12(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12) _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12
#define ARGS_13(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13) _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13
#define ARGS_14(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14) _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14
#define ARGS_15(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15) _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15
#define ARGS_16(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16) _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16
#define ARGS(...) CAT(ARGS_,ARGS_SIZE(__VA_ARGS__))(__VA_ARGS__)

// === ARGS_SIZE ===
#define ARGS_SIZE_IMPL(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,N,...) N
#define ARGS_SIZE(...) ARGS_SIZE_IMPL(__VA_OPT__(__VA_ARGS__,)16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

// === ARGS_SIZE ===
#define ARGS_EMPTY(...) NOT(BOOL(CAT(__VA_OPT__(1),0)))

// === ARGS_TO_TUPLE ===
#define ARGS_TO_TUPLE(...) (ARGS(__VA_ARGS__))

// === ARGS_ELEMENT ===
#define ARGS_ELEMENT_0(_1,...) _1
#define ARGS_ELEMENT_1(_1,_2,...) _2
#define ARGS_ELEMENT_2(_1,_2,_3,...) _3
#define ARGS_ELEMENT_3(_1,_2,_3,_4,...) _4
#define ARGS_ELEMENT_4(_1,_2,_3,_4,_5,...) _5
#define ARGS_ELEMENT_5(_1,_2,_3,_4,_5,_6,...) _6
#define ARGS_ELEMENT_6(_1,_2,_3,_4,_5,_6,_7,...) _7
#define ARGS_ELEMENT_7(_1,_2,_3,_4,_5,_6,_7,_8,...) _8
#define ARGS_ELEMENT_8(_1,_2,_3,_4,_5,_6,_7,_8,_9,...) _9
#define ARGS_ELEMENT_9(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,...) _10
#define ARGS_ELEMENT_10(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,...) _11
#define ARGS_ELEMENT_11(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,...) _12
#define ARGS_ELEMENT_12(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,...) _13
#define ARGS_ELEMENT_13(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,...) _14
#define ARGS_ELEMENT_14(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,...) _15
#define ARGS_ELEMENT_15(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16) _16
#define ARGS_ELEMENT(N,...) CAT(ARGS_ELEMENT_,N)(__VA_ARGS__)

// === ARGS_TAIL ===
#define ARGS_TAIL_0(_1,...) __VA_ARGS__
#define ARGS_TAIL_1()
#define ARGS_TAIL(...) CAT(ARGS_TAIL_,ARGS_EMPTY(__VA_ARGS__))(__VA_ARGS__)

// === ARGS_FOREACH ===
#define ARGS_FOREACH_1(F,_1) F(_1)
#define ARGS_FOREACH_2(F,_1,_2) F(_1) F(_2)
#define ARGS_FOREACH_3(F,_1,_2,_3) F(_1) F(_2) F(_3)
#define ARGS_FOREACH_4(F,_1,_2,_3,_4) F(_1) F(_2) F(_3) F(_4)
#define ARGS_FOREACH_5(F,_1,_2,_3,_4,_5) F(_1) F(_2) F(_3) F(_4) F(_5)
#define ARGS_FOREACH_6(F,_1,_2,_3,_4,_5,_6) F(_1) F(_2) F(_3) F(_4) F(_5) F(_6)
#define ARGS_FOREACH_7(F,_1,_2,_3,_4,_5,_6,_7) F(_1) F(_2) F(_3) F(_4) F(_5) F(_6) F(_7)
#define ARGS_FOREACH_8(F,_1,_2,_3,_4,_5,_6,_7,_8) F(_1) F(_2) F(_3) F(_4) F(_5) F(_6) F(_7) F(_8)
#define ARGS_FOREACH_9(F,_1,_2,_3,_4,_5,_6,_7,_8,_9) F(_1) F(_2) F(_3) F(_4) F(_5) F(_6) F(_7) F(_8) F(_9)
#define ARGS_FOREACH_10(F, _1,_2,_3,_4,_5,_6,_7,_8,_9,_10) F(_1) F(_2) F(_3) F(_4) F(_5) F(_6) F(_7) F(_8) F(_9) F(_10)
#define ARGS_FOREACH_11(F,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11) F(_1) F(_2) F(_3) F(_4) F(_5) F(_6) F(_7) F(_8) F(_9) F(_10) F(_11)
#define ARGS_FOREACH_12(F,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12) F(_1) F(_2) F(_3) F(_4) F(_5) F(_6) F(_7) F(_8) F(_9) F(_10) F(_11) F(_12)
#define ARGS_FOREACH_13(F,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13) F(_1) F(_2) F(_3) F(_4) F(_5) F(_6) F(_7) F(_8) F(_9) F(_10) F(_11) F(_12) F(_13)
#define ARGS_FOREACH_14(F,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14) F(_1) F(_2) F(_3) F(_4) F(_5) F(_6) F(_7) F(_8) F(_9) F(_10) F(_11) F(_12) F(_13) F(_14)
#define ARGS_FOREACH_15(F,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15) F(_1) F(_2) F(_3) F(_4) F(_5) F(_6) F(_7) F(_8) F(_9) F(_10) F(_11) F(_12) F(_13) F(_14) F(_15)
#define ARGS_FOREACH_16(F,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16) F(_1) F(_2) F(_3) F(_4) F(_5) F(_6) F(_7) F(_8) F(_9) F(_10) F(_11) F(_12) F(_13) F(_14) F(_15) F(_16)
#define ARGS_FOREACH(...) CAT(ARGS_FOREACH_,ARGS_SIZE(__VA_ARGS__))(__VA_ARGS__)

// === ARGS_MAP ===
#define ARGS_MAP_1(M,_1) M(_1)
#define ARGS_MAP_2(M,_1,_2) M(_1),M(_2)
#define ARGS_MAP_3(M,_1,_2,_3) M(_1),M(_2),M(_3)
#define ARGS_MAP_4(M,_1,_2,_3,_4) M(_1),M(_2),M(_3),M(_4)
#define ARGS_MAP_5(M,_1,_2,_3,_4,_5) M(_1),M(_2),M(_3),M(_4),M(_5)
#define ARGS_MAP_6(M,_1,_2,_3,_4,_5,_6) M(_1),M(_2),M(_3),M(_4),M(_5),M(_6)
#define ARGS_MAP_7(M,_1,_2,_3,_4,_5,_6,_7) M(_1),M(_2),M(_3),M(_4),M(_5),M(_6),M(_7)
#define ARGS_MAP_8(M,_1,_2,_3,_4,_5,_6,_7,_8) M(_1),M(_2),M(_3),M(_4),M(_5),M(_6),M(_7),M(_8)
#define ARGS_MAP_9(M,_1,_2,_3,_4,_5,_6,_7,_8,_9) M(_1),M(_2),M(_3),M(_4),M(_5),M(_6),M(_7),M(_8),M(_9)
#define ARGS_MAP_10(M,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10) M(_1),M(_2),M(_3),M(_4),M(_5),M(_6),M(_7),M(_8),M(_9),M(_10)
#define ARGS_MAP_11(M,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11) M(_1),M(_2),M(_3),M(_4),M(_5),M(_6),M(_7),M(_8),M(_9),M(_10),M(_11)
#define ARGS_MAP_12(M,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12) M(_1),M(_2),M(_3),M(_4),M(_5),M(_6),M(_7),M(_8),M(_9),M(_10),M(_11),M(_12)
#define ARGS_MAP_13(M,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13) M(_1),M(_2),M(_3),M(_4),M(_5),M(_6),M(_7),M(_8),M(_9),M(_10),M(_11),M(_12),M(_13)
#define ARGS_MAP_14(M,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14) M(_1),M(_2),M(_3),M(_4),M(_5),M(_6),M(_7),M(_8),M(_9),M(_10),M(_11),M(_12),M(_13),M(_14)
#define ARGS_MAP_15(M,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15) M(_1),M(_2),M(_3),M(_4),M(_5),M(_6),M(_7),M(_8),M(_9),M(_10),M(_11),M(_12),M(_13),M(_14),M(_15)
#define ARGS_MAP_16(M,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16) M(_1),M(_2),M(_3),M(_4),M(_5),M(_6),M(_7),M(_8),M(_9),M(_10),M(_11),M(_12),M(_13),M(_14),M(_15),M(_16)
#define ARGS_MAP(M,...) CAT(ARGS_MAP_,ARGS_SIZE(__VA_ARGS__))(M,__VA_ARGS__)

// === TUPLE ===
#define TUPLE(TUPLE) ARGS_TO_TUPLE TUPLE

// === TUPLE_SIZE ===
#define TUPLE_SIZE(TUPLE) ARGS_SIZE(ARGS TUPLE)

// === TUPLE_TO_ARGS ===
#define TUPLE_TO_ARGS(TUPLE) ARGS TUPLE

// === TUPLE_ELEMENT ===
#define TUPLE_ELEMENT(N,TUPLE) ARGS_ELEMENT(N,ARGS TUPLE)

// === TUPLE_FOREACH ===
#define TUPLE_FOREACH(F,TUPLE) ARGS_FOREACH(F,ARGS TUPLE)

// === TUPLE_MAP ===
#define TUPLE_MAP(M,TUPLE) ARGS_TO_TUPLE(ARGS_MAP(M,ARGS TUPLE))

// === PROBE/CHECK ===
#define PROBE() ~,1
#define CHECK(...) ARGS_ELEMENT_1(__VA_ARGS__,0)

// === NOT ===
#define NOT_0 PROBE()
#define NOT(X) CHECK(CAT(NOT_,X))

// === BOOL ===
#define BOOL(X) NOT(NOT(X))

// === IF ===
#define IF_1(COND,T) ARGS_ELEMENT(BOOL(COND),,T)
#define IF_2(COND,T,F) ARGS_ELEMENT(BOOL(COND),F,T)
#define IF(COND,...) CAT(IF_,ARGS_SIZE(__VA_ARGS__))(COND,__VA_ARGS__)

// clang-format on
