#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OP_PUSH,
    OP_POP,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_HALT,
    OP_STORE,
    OP_LOAD,
    OP_DEL,
    OP_COUNT  // keep last: number of opcodes
} Operations;

#ifdef __cplusplus
}
#endif