#include <stdio.h>
#include <stdlib.h>

#include "errors.h"

void throwError(const char* msg) {
    fprintf(stderr, "\033[31mError!\n%s\n\033[0m", msg);
    exit(1);
}

void throwWarning(const char* msg) { fprintf(stderr,"\033[33mWarning!\n%s\n\033[0m", msg); }

void throwInfo(const char* msg) { printf("\033[36mInformation:\n%s\n\033[0m", msg); }