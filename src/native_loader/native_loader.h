#ifndef NATIVE_LOADER_H
#define NATIVE_LOADER_H
#include <stddef.h>

extern char **nativeLibraryPaths;
extern size_t nativeLibraryCount;

void loadNativeLibraries(const char *path);
#endif
