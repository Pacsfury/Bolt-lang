#pragma once
#include <stdio.h>
#include <stdlib.h>

#if defined(__GNUC__) || defined(__clang__)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define NOINLINE __attribute__((noinline, cold))
#else
#define UNLIKELY(x) (x)
#define NOINLINE
#endif

typedef struct {
    int* base;   // start of buffer
    int* sp;     // next free slot (one past top element)
    int* limit;  // one past end of buffer
} DynamicStack;

static void oom(void) {
    fprintf(stderr, "BVM Memory Error: Couldn't allocate stack.\n");
    exit(1);
}

static void init_stack(DynamicStack* s, size_t initial_capacity) {
    if (initial_capacity == 0)
        initial_capacity = 16;
    s->base = malloc(initial_capacity * sizeof *s->base);
    if (!s->base)
        oom();
    s->sp = s->base;
    s->limit = s->base + initial_capacity;
}

// only runs on growth
static NOINLINE void expand_stack(DynamicStack* s) {
    size_t size = (size_t)(s->sp - s->base);
    size_t cap = (size_t)(s->limit - s->base);

    if (cap > SIZE_MAX / 2 / sizeof *s->base)
        oom();  // overflow guard
    size_t new_cap = cap * 2;

    int* nb = realloc(s->base, new_cap * sizeof *nb);
    if (!nb)
        oom();

    s->base = nb;
    s->sp = nb + size;
    s->limit = nb + new_cap;
}

static inline void push(DynamicStack* s, int val) {
    if (UNLIKELY(s->sp == s->limit))
        expand_stack(s);
    *s->sp++ = val;
}

static inline int pop(DynamicStack* s) { return *--s->sp; }
static inline int peek(const DynamicStack* s) { return s->sp[-1]; }
static inline int peek_at(const DynamicStack* s, int offset) { return s->sp[-1 - offset]; }

static inline int stack_empty(const DynamicStack* s) { return s->sp == s->base; }
static inline size_t stack_size(const DynamicStack* s) { return (size_t)(s->sp - s->base); }

static void free_stack(DynamicStack* s) {
    free(s->base);
    s->base = s->sp = s->limit = NULL;
}