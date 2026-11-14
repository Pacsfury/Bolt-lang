#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int cpool_init(const uint8_t* data, size_t size);
int cpool_load_file(const char* path);
void cpool_destroy(void);
int cpool_append(const char* value);
void cpool_delete(int idx);
const char* cpool_read(int idx);

static inline int32_t read_int(const uint8_t** ip) {
    uint32_t value =
        ((uint32_t)(*ip)[0] << 24) | ((uint32_t)(*ip)[1] << 16) |
        ((uint32_t)(*ip)[2] << 8)  |  (uint32_t)(*ip)[3];
    *ip += 4;

    if (value <= INT32_MAX)
        return (int32_t)value;
    return -1 - (int32_t)(UINT32_MAX - value);
}

void execute(DynamicStack* restrict s, const uint8_t* restrict code);
