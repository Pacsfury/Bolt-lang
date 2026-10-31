#if !defined(_WIN32)
#define _POSIX_C_SOURCE 199309L
#endif

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "vector.h"
#include "executor.h"
#include "errors.h"

#ifdef _WIN32
#include <windows.h>
typedef LARGE_INTEGER TimerPoint;

static LARGE_INTEGER timer_frequency;

static int timer_init(void) {
    return QueryPerformanceFrequency(&timer_frequency) != 0 &&
           timer_frequency.QuadPart > 0;
}

static int timer_now(TimerPoint* point) {
    return QueryPerformanceCounter(point) != 0;
}

static uint64_t elapsed_nanoseconds(TimerPoint start, TimerPoint end) {
    uint64_t elapsed_ticks = (uint64_t)(end.QuadPart - start.QuadPart);
    uint64_t frequency = (uint64_t)timer_frequency.QuadPart;
    uint64_t elapsed_seconds = elapsed_ticks / frequency;
    uint64_t remaining_ticks = elapsed_ticks % frequency;

    return elapsed_seconds * 1000000000ULL +
           remaining_ticks * 1000000000ULL / frequency;
}
#else
#include <time.h>
typedef struct timespec TimerPoint;

static int timer_init(void) {
    return 1;
}

static int timer_now(TimerPoint* point) {
    return clock_gettime(CLOCK_MONOTONIC, point) == 0;
}

static uint64_t elapsed_nanoseconds(TimerPoint start, TimerPoint end) {
    int64_t seconds = (int64_t)(end.tv_sec - start.tv_sec);
    int64_t nanoseconds = (int64_t)(end.tv_nsec - start.tv_nsec);
    return (uint64_t)(seconds * 1000000000LL + nanoseconds);
}
#endif

static void report_timer_error(const char* operation) {
    throwError(strcat("BVM Timer Error: Failed operation '", operation, "'."));
}

int main(void) {
    FILE* file = fopen("main.bvm", "rb");
    if (!file) {
        throwError("BVM File Error: Could't open 'main.bvm'");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        throwError("BVM File Error: Blank or unvalid file");
        fclose(file);
        return 1;
    }

    uint8_t* bytecode = (uint8_t*)malloc(file_size);
    if (!bytecode) {
        throwError("BVM Memory Error: Missing space for loading bytecode");
        fclose(file);
        return 1;
    }

    size_t bytes_read = fread(bytecode, 1, file_size, file);
    fclose(file);

    if (bytes_read != (size_t)file_size) {
        throwError("BVM File Error: couln't read file content");
        free(bytecode);
        return 1;
    }

    DynamicStack stack;
    init_stack(&stack, 64);

    const uint8_t* code = bytecode;
    if (read_int(&code) != 1112493140) /* Does it start with BOLT? */ {
        throwError("BVM Syntax Error: Missing 'BOLT' at file start\n");
        free_stack(&stack);
        free(bytecode);
        return 1;
    }

    TimerPoint start, end;
    if (!timer_init()) {
        report_timer_error("Timer initialization");
        free_stack(&stack);
        free(bytecode);
        return 1;
    }
    if (!timer_now(&start)) {
        report_timer_error("Timer start");
        free_stack(&stack);
        free(bytecode);
        return 1;
    }

    execute(&stack, code);

    if (!timer_now(&end)) {
        report_timer_error("Timer end");
        free_stack(&stack);
        free(bytecode);
        return 1;
    }

    int result = peek(&stack);

    uint64_t elapsed = elapsed_nanoseconds(start, end);

    printf("\nVM Execution time: %" PRIu64 " nanoseconds.\n", elapsed);
    printf("Or: %" PRIu64 ".%04" PRIu64 " ms.\n",
           elapsed / 1000000, (elapsed % 1000000) / 100);

    free_stack(&stack);
    free(bytecode);

    return result;
}
