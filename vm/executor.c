#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../common/bytecode.h"
#include "vector.h"
#include "executor.h"

typedef char compile_time_assert_opcodes[(OP_COUNT <= 256) ? 1 : -1];

void execute(DynamicStack* restrict s, const uint8_t* restrict code) {
    const uint8_t* ip = code;
    int* sp = s->sp;
    int* limit = s->limit;
    int* base = s->base;
    (void)base;

#if defined(__GNUC__) || defined(__clang__)
    // Every slot defaults to the error handler; real opcodes override it.
    static const void* const dispatch[256] = {
        [0 ... 255] = &&L_BAD,  // GNU range initializer
        [OP_PUSH] = &&L_PUSH,  [OP_POP] = &&L_POP, [OP_ADD] = &&L_ADD, [OP_SUB] = &&L_SUB, [OP_MUL] = &&L_MUL, [OP_DIV] = &&L_DIV, [OP_HALT] = &&L_HALT,
    };
#define NEXT() goto* dispatch[*ip++]
#define CASE(name) L_##name:
    NEXT();
#else
#define NEXT() continue
#define CASE(name) case OP_##name:
    for (;;) switch (*ip++) {
            default:
                goto bad;  // portable fallback
#endif

    CASE(PUSH) {
        if (UNLIKELY(sp == limit)) {
            s->sp = sp;
            expand_stack(s);
            base = s->base;
            sp = s->sp;
            limit = s->limit;
        }
        *sp++ = read_int(&ip);
        NEXT();
    }

    CASE(POP) {
        assert(sp > base);
        --sp;
        NEXT();
    }

    CASE(ADD) {
        assert(sp - base >= 2);
        sp[-2] = (int)((unsigned)sp[-2] + (unsigned)sp[-1]);
        --sp;
        NEXT();
    }

    CASE(SUB) {
        assert(sp - base >= 2);
        sp[-2] = (int)((unsigned)sp[-2] - (unsigned)sp[-1]);
        --sp;
        NEXT();
    }

    CASE(MUL) {
        assert(sp - base >= 2);
        sp[-2] = (int)((unsigned)sp[-2] * (unsigned)sp[-1]);
        --sp;
        NEXT();
    }

    CASE(DIV) {
        assert((sp - base >= 2) && (unsigned)sp[-1] != 0);
        sp[-2] = (int)((unsigned)sp[-2] / (unsigned)sp[-1]);
        --sp;
        NEXT();
    }


    CASE(HALT) {
        s->sp = sp;
        return;
    }

#if defined(__GNUC__) || defined(__clang__)
L_BAD:
#else
        }
bad:
#endif
    s->sp = sp;
    fprintf(stderr, "BVM Error: invalid opcode at offset %td\n", ip - 1 - code);
    abort();
}