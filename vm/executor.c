#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../common/bytecode.h"
#include "vector.h"
#include "executor.h"

typedef char compile_time_assert_opcodes[(OP_COUNT <= 256) ? 1 : -1];

static char** cpool_entries = NULL;
static size_t cpool_count = 0;
static size_t cpool_capacity = 0;

static int cpool_reserve(size_t required) {
    if (required <= cpool_capacity)
        return 0;
    size_t max_capacity = SIZE_MAX / sizeof(*cpool_entries);
    if (required > max_capacity) {
        fprintf(stderr, "BVM Memory Error: constant pool is too large.\n");
        return -1;
    }

    size_t capacity = cpool_capacity ? cpool_capacity : 16;
    while (capacity < required) {
        if (capacity > max_capacity / 2) {
            capacity = required;
            break;
        }
        capacity *= 2;
    }

    char** entries = realloc(cpool_entries, capacity * sizeof(*entries));
    if (!entries) {
        fprintf(stderr, "BVM Memory Error: couldn't allocate constant pool.\n");
        return -1;
    }
    cpool_entries = entries;
    cpool_capacity = capacity;
    return 0;
}

static int cpool_append_n(const char* value, size_t length) {
    if (cpool_count >= INT_MAX) {
        fprintf(stderr, "BVM CPOOL Error: constant pool is full.\n");
        return -1;
    }
    if (cpool_reserve(cpool_count + 2) != 0)
        return -1;

    char* copy = malloc(length + 1);
    if (!copy) {
        fprintf(stderr, "BVM Memory Error: couldn't allocate constant pool value.\n");
        return -1;
    }
    if (length)
        memcpy(copy, value, length);
    copy[length] = '\0';

    size_t index = ++cpool_count;
    cpool_entries[index] = copy;
    return (int)index;
}

int cpool_append(const char* value) {
    if (!value) {
        fprintf(stderr, "BVM CPOOL Error: cannot append a null value.\n");
        return -1;
    }
    return cpool_append_n(value, strlen(value));
}

void cpool_delete(int idx) {
    if (idx <= 0 || (size_t)idx > cpool_count || !cpool_entries[idx]) {
        fprintf(stderr, "BVM CPOOL Error: index %d does not exist.\n", idx);
        return;
    }
    free(cpool_entries[idx]);
    cpool_entries[idx] = NULL;
}

const char* cpool_read(int idx) {
    if (idx <= 0 || (size_t)idx > cpool_count || !cpool_entries[idx]) {
        fprintf(stderr, "BVM CPOOL Error: index %d does not exist.\n", idx);
        return NULL;
    }
    return cpool_entries[idx];
}

void cpool_destroy(void) {
    for (size_t i = 1; i <= cpool_count; ++i)
        free(cpool_entries[i]);
    free(cpool_entries);
    cpool_entries = NULL;
    cpool_count = 0;
    cpool_capacity = 0;
}

int cpool_init(const uint8_t* data, size_t size) {
    if (size > 0 && !data) {
        fprintf(stderr, "BVM CPOOL Error: missing constant pool data.\n");
        return -1;
    }

    cpool_destroy();
    size_t start = 0;
    while (start < size) {
        size_t end = start;
        while (end < size && data[end] != '\n')
            ++end;
        size_t length = end - start;
        if (length > 0 && data[start + length - 1] == '\r')
            --length;
        if (cpool_append_n((const char*)data + start, length) < 0) {
            cpool_destroy();
            return -1;
        }
        start = end < size ? end + 1 : end;
    }
    return 0;
}

int cpool_load_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        if (errno == ENOENT) {
            cpool_destroy();
            return 0;
        }
        perror("BVM CPOOL Error: couldn't open constant pool");
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        perror("BVM CPOOL Error: couldn't seek constant pool");
        fclose(file);
        return -1;
    }
    long file_size = ftell(file);
    if (file_size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        perror("BVM CPOOL Error: couldn't determine constant pool size");
        fclose(file);
        return -1;
    }
    uint8_t* data = file_size ? malloc((size_t)file_size) : NULL;
    if (file_size && !data) {
        fprintf(stderr, "BVM Memory Error: couldn't allocate constant pool data.\n");
        fclose(file);
        return -1;
    }
    if (file_size && fread(data, 1, (size_t)file_size, file) != (size_t)file_size) {
        perror("BVM CPOOL Error: couldn't read constant pool");
        free(data);
        fclose(file);
        return -1;
    }
    if (fclose(file) != 0) {
        perror("BVM CPOOL Error: couldn't close constant pool");
        free(data);
        return -1;
    }

    int status = cpool_init(data, (size_t)file_size);
    free(data);
    return status;
}

void execute(DynamicStack* restrict s, const uint8_t* restrict code) {
    const uint8_t* ip = code;
    DynamicStack dataStack;
    init_stack(&dataStack, 16);
    int* sp = s->sp;
    int* limit = s->limit;
    int* base = s->base;
    (void)base;

#if defined(__GNUC__) || defined(__clang__)
    // Every slot defaults to the error handler; real opcodes override it.
    static const void* const dispatch[256] = {
        [0 ... 255] = &&L_BAD,  // GNU range initializer
        [OP_PUSH] = &&L_PUSH,  [OP_POP] = &&L_POP, [OP_ADD] = &&L_ADD,   [OP_SUB] = &&L_SUB,
        [OP_MUL] = &&L_MUL,    [OP_DIV] = &&L_DIV, [OP_HALT] = &&L_HALT, [OP_STORE] = &&L_STORE, [OP_LOAD] = &&L_LOAD,
        [OP_DEL] = &&L_DEL, [OP_PRINT] = &&L_PRINT, [OP_EQU] = &&L_EQU, [OP_READ] = &&L_READ,
        [OP_PRINT_VAR] = &&L_PRINT_VAR
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
        free_stack(&dataStack);
        return;
    }

    CASE(STORE) {
        int slot = read_int(&ip);
        assert(slot >= 0);
        assert(sp > base);
        while (UNLIKELY(dataStack.base + slot >= dataStack.limit)) {
            expand_stack(&dataStack);
        }
        if (dataStack.base + slot >= dataStack.sp) {
            dataStack.sp = dataStack.base + slot + 1;
        }
        dataStack.base[slot] = *--sp;
        NEXT();
    }

    CASE(LOAD) {
        int slot = read_int(&ip);
        assert(slot >= 0 && dataStack.base + slot < dataStack.sp);
        if (UNLIKELY(sp == limit)) {
            s->sp = sp;
            expand_stack(s);
            base = s->base;
            sp = s->sp;
            limit = s->limit;
        }
        *sp++ = dataStack.base[slot];
        NEXT();
    }


    CASE(DEL) {
        assert(!stack_empty(&dataStack));
        --dataStack.sp;
        NEXT();
    }

    CASE(PRINT) {
        const char* data = cpool_read(read_int(&ip));
        if (!data) {
            s->sp = sp;
            free_stack(&dataStack);
            abort();
        }
        puts(data);
        NEXT();
    }

    CASE(PRINT_VAR) {
        assert(sp > base);
        const char* data = cpool_read(*--sp);
        if (!data) {
            s->sp = sp;
            free_stack(&dataStack);
            abort();
        }
        puts(data);
        NEXT();
    }

    CASE(READ) {
        char buf[1024];
        char *line = buf;
        size_t cap = sizeof(buf), len = 0;
        int idx = 0;

        while (fgets(line + len, cap - len, stdin)) {
            len += strlen(line + len);
            if (len > 0 && line[len - 1] == '\n') break;
            char *tmp = realloc(line == buf ? NULL : line, cap *= 2);
            if (!tmp) break;
            if (line == buf) memcpy(tmp, buf, len + 1);
            line = tmp;
        }
        if (len > 0) {
            line[strcspn(line, "\r\n")] = '\0';
            idx = cpool_append(line);
        }
        if (line != buf) free(line);
        if (UNLIKELY(sp == limit)) {
            s->sp = sp;
            expand_stack(s);
            base = s->base;
            sp = s->sp;
            limit = s->limit;
        }
        *sp++ = idx;
        NEXT();
    }

    CASE(EQU) {
        assert(sp - base >= 2);
        int right = *--sp;
        int left = *--sp;
        *sp++ = left == right;
        NEXT();
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
