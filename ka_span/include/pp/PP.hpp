#pragma once

// clang-format off

// === STRINGIFY ===
#define STRINGIFY(x) #x

// === TOSTRING ===
#define TOSTRING(x) STRINGIFY(x)

// === CAT ===
#define CAT_IMPL(a,b) a ## b
#define CAT(a,b) CAT_IMPL(a,b)

// === ARGS_SIZE ===
#define ARGS_SIZE_IMPL(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,N,...) N
#define ARGS_SIZE(...) ARGS_SIZE_IMPL(__VA_OPT__(__VA_ARGS__,)16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

// === ARGS_TO_TUPLE ===
#define ARGS_TO_TUPLE_0() ()
#define ARGS_TO_TUPLE_1(_1) (_1)
#define ARGS_TO_TUPLE_2(_1,_2) (_1,_2)
#define ARGS_TO_TUPLE_3(_1,_2,_3) (_1,_2,_3)
#define ARGS_TO_TUPLE_4(_1,_2,_3,_4) (_1,_2,_3,_4)
#define ARGS_TO_TUPLE_5(_1,_2,_3,_4,_5) (_1,_2,_3,_4,_5)
#define ARGS_TO_TUPLE_6(_1,_2,_3,_4,_5,_6) (_1,_2,_3,_4,_5,_6)
#define ARGS_TO_TUPLE_7(_1,_2,_3,_4,_5,_6,_7) (_1,_2,_3,_4,_5,_6,_7)
#define ARGS_TO_TUPLE_8(_1,_2,_3,_4,_5,_6,_7,_8) (_1,_2,_3,_4,_5,_6,_7,_8)
#define ARGS_TO_TUPLE_9(_1,_2,_3,_4,_5,_6,_7,_8,_9) (_1,_2,_3,_4,_5,_6,_7,_8,_9)
#define ARGS_TO_TUPLE_10(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10) (_1,_2,_3,_4,_5,_6,_7,_8,_9,_10)
#define ARGS_TO_TUPLE_11(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11) (_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11)
#define ARGS_TO_TUPLE_12(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12) (_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12)
#define ARGS_TO_TUPLE_13(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13) (_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13)
#define ARGS_TO_TUPLE_14(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14) (_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14)
#define ARGS_TO_TUPLE_15(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15) (_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15)
#define ARGS_TO_TUPLE_16(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16) (_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16)
#define ARGS_TO_TUPLE_N(N,...) ARGS_TO_TUPLE_ ## N(__VA_ARGS__)
#define ARGS_TO_TUPLE_IMPL(N,...) ARGS_TO_TUPLE_N(N,__VA_ARGS__)
#define ARGS_TO_TUPLE(...) ARGS_TO_TUPLE_IMPL(ARGS_SIZE(__VA_ARGS__),__VA_ARGS__)

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
#define ARGS_ELEMENT(N,...) ARGS_ELEMENT_ ## N(__VA_ARGS__)

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
#define ARGS_FOREACH_N(N,F,...) ARGS_FOREACH_ ## N(F,__VA_ARGS__)
#define ARGS_FOREACH_IMPL(N,F,...) ARGS_FOREACH_N(N,F,__VA_ARGS__)
#define ARGS_FOREACH(F,...) ARGS_FOREACH_IMPL(ARGS_SIZE(__VA_ARGS__),F,__VA_ARGS__)

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
#define ARGS_MAP_N(N,M,...) ARGS_MAP_ ## N(M,__VA_ARGS__)
#define ARGS_MAP_IMPL(N,M,...) ARGS_MAP_N(N,M,__VA_ARGS__)
#define ARGS_MAP(M,...) ARGS_MAP_IMPL(ARGS_SIZE(__VA_ARGS__),M,__VA_ARGS__)

// === TUPLE_SIZE ===
#define TUPLE_SIZE(TUPLE) ARGS_SIZE(TUPLE_TO_ARGS(TUPLE))

// === TUPLE_TO_ARGS ===
#define TUPLE_TO_ARGS_IMPL(...) __VA_ARGS__
#define TUPLE_TO_ARGS(TUPLE) TUPLE_TO_ARGS_IMPL TUPLE

// === TUPLE_ELEMENT ===
#define TUPLE_ELEMENT(N,TUPLE) ARGS_ELEMENT(N,TUPLE_TO_ARGS(TUPLE))

// === TUPLE_FOREACH ===
#define TUPLE_FOREACH(F,TUPLE) ARGS_FOREACH(F,TUPLE_TO_ARGS(TUPLE))

// === TUPLE_MAP ===
#define TUPLE_MAP(M,TUPLE) ARGS_TO_TUPLE(ARGS_MAP(M,TUPLE_TO_ARGS(TUPLE)))

// === TUPLE_ZIP ===
#define TUPLE_ZIP_1(_1a,_1b) (_1a,_1b)
#define TUPLE_ZIP_2(_1a,_2a,_1b,_2b) (_1a,_1b),(_2a,_2b)
#define TUPLE_ZIP_3(_1a,_2a,_3a,_1b,_2b,_3b) (_1a,_1b),(_2a,_2b),(_3a,_3b)
#define TUPLE_ZIP_4(_1a,_2a,_3a,_4a,_1b,_2b,_3b,_4b) (_1a,_1b),(_2a,_2b),(_3a,_3b),(_4a,_4b)
#define TUPLE_ZIP_5(_1a,_2a,_3a,_4a,_5a,_1b,_2b,_3b,_4b,_5b) (_1a,_1b),(_2a,_2b),(_3a,_3b),(_4a,_4b),(_5a,_5b)
#define TUPLE_ZIP_6(_1a,_2a,_3a,_4a,_5a,_6a,_1b,_2b,_3b,_4b,_5b,_6b) (_1a,_1b),(_2a,_2b),(_3a,_3b),(_4a,_4b),(_5a,_5b),(_6a,_6b)
#define TUPLE_ZIP_7(_1a,_2a,_3a,_4a,_5a,_6a,_7a,_1b,_2b,_3b,_4b,_5b,_6b,_7b) (_1a,_1b),(_2a,_2b),(_3a,_3b),(_4a,_4b),(_5a,_5b),(_6a,_6b),(_7a,_7b)
#define TUPLE_ZIP_8(_1a,_2a,_3a,_4a,_5a,_6a,_7a,_8a,_1b,_2b,_3b,_4b,_5b,_6b,_7b,_8b) (_1a,_1b),(_2a,_2b),(_3a,_3b),(_4a,_4b),(_5a,_5b),(_6a,_6b),(_7a,_7b),(_8a,_8b)
#define TUPLE_ZIP_9(_1a,_2a,_3a,_4a,_5a,_6a,_7a,_8a,_9a,_1b,_2b,_3b,_4b,_5b,_6b,_7b,_8b,_9b) (_1a,_1b),(_2a,_2b),(_3a,_3b),(_4a,_4b),(_5a,_5b),(_6a,_6b),(_7a,_7b),(_8a,_8b),(_9a,_9b)
#define TUPLE_ZIP_10(_1a,_2a,_3a,_4a,_5a,_6a,_7a,_8a,_9a,_10a,_1b,_2b,_3b,_4b,_5b,_6b,_7b,_8b,_9b,_10b) (_1a,_1b),(_2a,_2b),(_3a,_3b),(_4a,_4b),(_5a,_5b),(_6a,_6b),(_7a,_7b),(_8a,_8b),(_9a,_9b),(_10a,_10b)
#define TUPLE_ZIP_11(_1a,_2a,_3a,_4a,_5a,_6a,_7a,_8a,_9a,_10a,_11a,_1b,_2b,_3b,_4b,_5b,_6b,_7b,_8b,_9b,_10b,_11b) (_1a,_1b),(_2a,_2b),(_3a,_3b),(_4a,_4b),(_5a,_5b),(_6a,_6b),(_7a,_7b),(_8a,_8b),(_9a,_9b),(_10a,_10b),(_11a,_11b)
#define TUPLE_ZIP_12(_1a,_2a,_3a,_4a,_5a,_6a,_7a,_8a,_9a,_10a,_11a,_12a,_1b,_2b,_3b,_4b,_5b,_6b,_7b,_8b,_9b,_10b,_11b,_12b) (_1a,_1b),(_2a,_2b),(_3a,_3b),(_4a,_4b),(_5a,_5b),(_6a,_6b),(_7a,_7b),(_8a,_8b),(_9a,_9b),(_10a,_10b),(_11a,_11b),(_12a,_12b)
#define TUPLE_ZIP_13(_1a,_2a,_3a,_4a,_5a,_6a,_7a,_8a,_9a,_10a,_11a,_12a,_13a,_1b,_2b,_3b,_4b,_5b,_6b,_7b,_8b,_9b,_10b,_11b,_12b,_13b) (_1a,_1b),(_2a,_2b),(_3a,_3b),(_4a,_4b),(_5a,_5b),(_6a,_6b),(_7a,_7b),(_8a,_8b),(_9a,_9b),(_10a,_10b),(_11a,_11b),(_12a,_12b),(_13a,_13b)
#define TUPLE_ZIP_14(_1a,_2a,_3a,_4a,_5a,_6a,_7a,_8a,_9a,_10a,_11a,_12a,_13a,_14a,_1b,_2b,_3b,_4b,_5b,_6b,_7b,_8b,_9b,_10b,_11b,_12b,_13b,_14b) (_1a,_1b),(_2a,_2b),(_3a,_3b),(_4a,_4b),(_5a,_5b),(_6a,_6b),(_7a,_7b),(_8a,_8b),(_9a,_9b),(_10a,_10b),(_11a,_11b),(_12a,_12b),(_13a,_13b),(_14a,_14b)
#define TUPLE_ZIP_15(_1a,_2a,_3a,_4a,_5a,_6a,_7a,_8a,_9a,_10a,_11a,_12a,_13a,_14a,_15a,_1b,_2b,_3b,_4b,_5b,_6b,_7b,_8b,_9b,_10b,_11b,_12b,_13b,_14b,_15b) (_1a,_1b),(_2a,_2b),(_3a,_3b),(_4a,_4b),(_5a,_5b),(_6a,_6b),(_7a,_7b),(_8a,_8b),(_9a,_9b),(_10a,_10b),(_11a,_11b),(_12a,_12b),(_13a,_13b),(_14a,_14b),(_15a,_15b)
#define TUPLE_ZIP_16(_1a,_2a,_3a,_4a,_5a,_6a,_7a,_8a,_9a,_10a,_11a,_12a,_13a,_14a,_15a,_16a,_1b,_2b,_3b,_4b,_5b,_6b,_7b,_8b,_9b,_10b,_11b,_12b,_13b,_14b,_15b,_16b) (_1a,_1b),(_2a,_2b),(_3a,_3b),(_4a,_4b),(_5a,_5b),(_6a,_6b),(_7a,_7b),(_8a,_8b),(_9a,_9b),(_10a,_10b),(_11a,_11b),(_12a,_12b),(_13a,_13b),(_14a,_14b),(_15a,_15b),(_16a,_16b)
#define TUPLE_ZIP_N(N,...) TUPLE_ZIP_ ## N(__VA_ARGS__)
#define TUPLE_ZIP_IMPL(N,...) TUPLE_ZIP_N(N,__VA_ARGS__)
#define TUPLE_ZIP(TUPLE_1,TUPLE_2) TUPLE_ZIP_IMPL(TUPLE_SIZE(TUPLE_1),TUPLE_TO_ARGS(TUPLE_1),TUPLE_TO_ARGS(TUPLE_2))

// clang-format on
