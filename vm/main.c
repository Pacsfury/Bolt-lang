#if !defined(_WIN32)
#define _POSIX_C_SOURCE 199309L
#endif

#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
typedef __int64 BoltFileOffset;

static int seek_file(FILE* file, uint64_t offset) {
    if (offset > INT64_MAX)
        return -1;
    return _fseeki64(file, (__int64)offset, SEEK_SET);
}

static int seek_file_end(FILE* file) {
    return _fseeki64(file, 0, SEEK_END);
}

static BoltFileOffset tell_file(FILE* file) {
    return _ftelli64(file);
}

static FILE* open_self_executable(void) {
    wchar_t path[32768];
    DWORD length = GetModuleFileNameW(NULL, path, (DWORD)(sizeof(path) / sizeof(path[0])));
    if (length == 0 || length >= sizeof(path) / sizeof(path[0]))
        return NULL;
    return _wfopen(path, L"rb");
}
#else
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
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
typedef off_t BoltFileOffset;

static int seek_file(FILE* file, uint64_t offset) {
    if (offset > (uint64_t)INT64_MAX)
        return -1;
    return fseeko(file, (off_t)offset, SEEK_SET);
}

static int seek_file_end(FILE* file) {
    return fseeko(file, 0, SEEK_END);
}

static BoltFileOffset tell_file(FILE* file) {
    return ftello(file);
}

static FILE* open_self_executable(void) {
    char path[4096];
    ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (length <= 0 || (size_t)length >= sizeof(path) - 1)
        return NULL;
    path[length] = '\0';
    return fopen(path, "rb");
}
#endif

void report_timer_error(const char* operation) {
    char buffer[256]; 
    
    snprintf(buffer, sizeof(buffer), "BVM Timer Error: Failed operation '%s'.", operation);
    throwError(buffer);
}

static uint64_t read_u64_le(const uint8_t* bytes) {
    uint64_t value = 0;
    for (unsigned int i = 0; i < 8; ++i)
        value |= (uint64_t)bytes[i] << (i * 8);
    return value;
}

static int load_embedded_package(uint8_t** bytecode, size_t* bytecode_size,
                                 uint8_t** cpool, size_t* cpool_size) {
    static const uint8_t magic[8] = {'B', 'O', 'L', 'T', 'P', 'K', 'G', '1'};
    uint8_t footer[40];
    FILE* file = open_self_executable();
    if (!file) {
        throwError("BVM File Error: couldn't open its own executable");
        return -1;
    }

    if (seek_file_end(file) != 0) {
        fclose(file);
        throwError("BVM File Error: couldn't seek to executable end");
        return -1;
    }
    BoltFileOffset file_size = tell_file(file);
    if (file_size < 0) {
        fclose(file);
        throwError("BVM File Error: couldn't determine executable size");
        return -1;
    }
    if ((uint64_t)file_size < sizeof(footer)) {
        fclose(file);
        return 0;
    }

    uint64_t footer_offset = (uint64_t)file_size - sizeof(footer);
    if (seek_file(file, footer_offset) != 0 ||
        fread(footer, 1, sizeof(footer), file) != sizeof(footer)) {
        fclose(file);
        throwError("BVM File Error: couldn't read executable footer");
        return -1;
    }
    if (memcmp(footer, magic, sizeof(magic)) != 0) {
        fclose(file);
        return 0;
    }

    uint64_t embedded_cpool_offset = read_u64_le(footer + 8);
    uint64_t embedded_cpool_size = read_u64_le(footer + 16);
    uint64_t embedded_bytecode_offset = read_u64_le(footer + 24);
    uint64_t embedded_bytecode_size = read_u64_le(footer + 32);
    if (embedded_cpool_offset > footer_offset ||
        embedded_cpool_size > footer_offset - embedded_cpool_offset ||
        embedded_cpool_offset + embedded_cpool_size != embedded_bytecode_offset ||
        embedded_bytecode_offset > footer_offset ||
        embedded_bytecode_size != footer_offset - embedded_bytecode_offset ||
        embedded_cpool_size > SIZE_MAX - 1 ||
        embedded_bytecode_size == 0 || embedded_bytecode_size > SIZE_MAX) {
        fclose(file);
        throwError("BVM Package Error: invalid embedded data offsets");
        return -1;
    }

    uint8_t* cpool_data = malloc((size_t)embedded_cpool_size + 1);
    uint8_t* bytecode_data = malloc((size_t)embedded_bytecode_size);
    if (!cpool_data || !bytecode_data) {
        free(cpool_data);
        free(bytecode_data);
        fclose(file);
        throwError("BVM Memory Error: couldn't allocate embedded package data");
        return -1;
    }

    if (seek_file(file, embedded_cpool_offset) != 0 ||
        (embedded_cpool_size > 0 &&
         fread(cpool_data, 1, (size_t)embedded_cpool_size, file) != (size_t)embedded_cpool_size) ||
        seek_file(file, embedded_bytecode_offset) != 0 ||
        fread(bytecode_data, 1, (size_t)embedded_bytecode_size, file) != (size_t)embedded_bytecode_size) {
        free(cpool_data);
        free(bytecode_data);
        fclose(file);
        throwError("BVM File Error: couldn't read embedded package data");
        return -1;
    }

    cpool_data[embedded_cpool_size] = '\0';
    fclose(file);
    *cpool = cpool_data;
    *cpool_size = (size_t)embedded_cpool_size;
    *bytecode = bytecode_data;
    *bytecode_size = (size_t)embedded_bytecode_size;
    return 1;
}

int main(void) {
    uint8_t* bytecode = NULL;
    uint8_t* cpool = NULL;
    size_t bytecode_size = 0;
    size_t cpool_size = 0;
    int package_status = load_embedded_package(&bytecode, &bytecode_size, &cpool, &cpool_size);
    if (package_status < 0)
        return 1;

    if (package_status > 0) {
        cpool_set_embedded(cpool, cpool_size);
    } else {
        FILE* file = fopen("main.bvm", "rb");
        if (!file) {
            throwError("BVM File Error: Could't open 'main.bvm'");
            return 1;
        }

        if (fseek(file, 0, SEEK_END) != 0) {
            fclose(file);
            throwError("BVM File Error: couldn't seek in 'main.bvm'");
            return 1;
        }
        long file_size = ftell(file);
        if (file_size <= 0 || fseek(file, 0, SEEK_SET) != 0) {
            fclose(file);
            throwError("BVM File Error: Blank or invalid file");
            return 1;
        }

        bytecode_size = (size_t)file_size;
        bytecode = malloc(bytecode_size);
        if (!bytecode) {
            fclose(file);
            throwError("BVM Memory Error: Missing space for loading bytecode");
            return 1;
        }

        if (fread(bytecode, 1, bytecode_size, file) != bytecode_size) {
            fclose(file);
            free(bytecode);
            throwError("BVM File Error: couldn't read file content");
            return 1;
        }
        fclose(file);
    }

    DynamicStack stack;
    init_stack(&stack, 64);

    const uint8_t* code = bytecode;
    if (read_int(&code) != 1112493140) /* Does it start with BOLT? */ {
        throwError("BVM Syntax Error: Missing 'BOLT' at file start\n");
        free_stack(&stack);
        free(bytecode);
        free(cpool);
        return 1;
    }

    TimerPoint start, end;
    if (!timer_init()) {
        report_timer_error("Timer initialization");
        free_stack(&stack);
        free(bytecode);
        free(cpool);
        return 1;
    }
    if (!timer_now(&start)) {
        report_timer_error("Timer start");
        free_stack(&stack);
        free(bytecode);
        free(cpool);
        return 1;
    }

    execute(&stack, code);

    if (!timer_now(&end)) {
        report_timer_error("Timer end");
        free_stack(&stack);
        free(bytecode);
        free(cpool);
        return 1;
    }

    int result = peek(&stack);

    uint64_t elapsed = elapsed_nanoseconds(start, end);

    printf("\nVM Execution time: %" PRIu64 " nanoseconds.\n", elapsed);
    printf("Or: %" PRIu64 ".%04" PRIu64 " ms.\n",
           elapsed / 1000000, (elapsed % 1000000) / 100);

    free_stack(&stack);
    free(bytecode);
    free(cpool);

    return result;
}
