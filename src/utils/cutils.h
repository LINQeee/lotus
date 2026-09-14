#ifndef CUTILS_H
#define CUTILS_H
#include <stddef.h>

void *mallocSafe(size_t size);

void *reallocSafe(void *p, size_t size);

void exitWithError(const char *format, ...);

#endif
