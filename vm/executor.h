#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../common/bytecode.h"

static inline int32_t read_int(const uint8_t** ip) {
    uint32_t value =
        (uint32_t)(*ip)[0] | ((uint32_t)(*ip)[1] << 8) | ((uint32_t)(*ip)[2] << 16) | ((uint32_t)(*ip)[3] << 24);
    *ip += 4;

    if (value <= INT32_MAX)
        return (int32_t)value;
    return -1 - (int32_t)(UINT32_MAX - value);
}

void execute(DynamicStack* restrict s, const uint8_t* restrict code);