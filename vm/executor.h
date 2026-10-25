#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static inline char* cpool_read(int a) {
    if (a <= 0) {
        fprintf(stderr, "BVM CPOOL Error: line number must be positive.\n");
        return NULL;
    }

    FILE* file = fopen("BVM-CPOOL", "rb");
    if (!file) {
        perror("BVM CPOOL Error: couldn't open 'BVM-CPOOL'");
        return NULL;
    }

    for (int line = 1; line < a; ++line) {
        int ch;
        do {
            ch = fgetc(file);
        } while (ch != '\n' && ch != EOF);

        if (ch == EOF) {
            if (ferror(file)) {
                perror("BVM CPOOL Error: couldn't read 'BVM-CPOOL'");
            } else {
                fprintf(stderr, "BVM CPOOL Error: line %d does not exist.\n", a);
            }
            fclose(file);
            return NULL;
        }
    }

    size_t capacity = 128;
    size_t length = 0;
    char* contents = malloc(capacity);
    if (!contents) {
        fprintf(stderr, "BVM Memory Error: couldn't allocate CPOOL line.\n");
        fclose(file);
        return NULL;
    }

    int ch;
    while ((ch = fgetc(file)) != '\n' && ch != EOF) {
        if (length + 1 >= capacity) {
            if (capacity > (size_t)-1 / 2) {
                fprintf(stderr, "BVM Memory Error: CPOOL line is too large.\n");
                free(contents);
                fclose(file);
                return NULL;
            }
            size_t new_capacity = capacity * 2;
            char* expanded = realloc(contents, new_capacity);
            if (!expanded) {
                fprintf(stderr, "BVM Memory Error: couldn't allocate CPOOL line.\n");
                free(contents);
                fclose(file);
                return NULL;
            }
            contents = expanded;
            capacity = new_capacity;
        }
        contents[length++] = (char)ch;
    }

    if (ch == EOF && ferror(file)) {
        perror("BVM CPOOL Error: couldn't read 'BVM-CPOOL'");
        free(contents);
        fclose(file);
        return NULL;
    }
    if (ch == EOF && length == 0) {
        fprintf(stderr, "BVM CPOOL Error: line %d does not exist.\n", a);
        free(contents);
        fclose(file);
        return NULL;
    }

    if (length > 0 && contents[length - 1] == '\r') {
        --length;
    }
    contents[length] = '\0';
    fclose(file);
    return contents;
}

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