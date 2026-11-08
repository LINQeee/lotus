#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void exitWithError(const char *format, ...) {
    va_list args;
    va_start(args, format);

    vfprintf(stderr, format, args);

    va_end(args);

    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

void *mallocSafe(size_t size) {
    void *p = malloc(size);
    if (!p) exitWithError("Allocation error, aborting program");
    return p;
}

void *reallocSafe(void *p, size_t size) {
    void *new_p = realloc(p, size);
    if (!new_p) {
        free(p);
        exitWithError("Reallocation error, aborting program");
    }
    return new_p;
}
