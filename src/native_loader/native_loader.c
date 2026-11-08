#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "../utils/cutils.h"

#ifdef _WIN32
#include <fileapi.h>
#include <windows.h>
#else
#include <dirent.h>
#include <dlfcn.h>
#endif

#ifdef _WIN32
#define LIB_EXT ".dll"
#elif __APPLE__
#define LIB_EXT ".dylib"
#else
#define LIB_EXT ".so"
#endif

#ifdef _WIN32
#define LOTUS_CALL __cdecl
#else
#define LOTUS_CALL
#endif

#include "../llvm/generator/expression/native_functions.h"
#include "native_loader.h"

char **nativeLibraryPaths;
size_t nativeLibraryCount;

typedef void(LOTUS_CALL *RegisterFunction)(const LotusFunctionInfo *info);

#ifdef _WIN32
static void loadWindows(const char *path) {
    char searchPath[512];
    sprintf(searchPath, "%s\\*%s", path, LIB_EXT);

    WIN32_FIND_DATAA data;
    HANDLE hFind = FindFirstFileA(searchPath, &data);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        char full[512];
        sprintf(full, "%s\\%s", path, data.cFileName);

        HMODULE lib = LoadLibraryA(full);
        if (!lib) {
            DWORD err = GetLastError();

            printf("Cannot load %s error=%lu\n", full, err);
            continue;
        }

        RegisterFunction init = (RegisterFunction)GetProcAddress(lib, "lotusRegister");

        if (!init) {
            printf("No lotusRegister in %s\n", full);
            continue;
        }
        init(prepareNativeFunction);
    } while (FindNextFileA(hFind, &data));

    FindClose(hFind);
}

#endif

#ifndef _WIN32

static void loadUnix(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) exitWithError("Cannot open library directory '%s': %s", path, strerror(errno));

    struct dirent *entry;

    while ((entry = readdir(dir))) {
        size_t length = strlen(entry->d_name);
        if (length < strlen(LIB_EXT) || strcmp(entry->d_name + length - strlen(LIB_EXT), LIB_EXT)) continue;

        size_t size = strlen(path) + length + 2;
        char *full = mallocSafe(size);
        snprintf(full, size, "%s/%s", path, entry->d_name);

        void *lib = dlopen(full, RTLD_NOW | RTLD_GLOBAL);
        if (!lib) exitWithError("Cannot load '%s': %s", full, dlerror());

        void (*init)(RegisterFunction);

        init = (void (*)(RegisterFunction))dlsym(lib, "lotusRegister");

        if (init) {
            init(prepareNativeFunction);
            nativeLibraryPaths = reallocSafe(nativeLibraryPaths, (nativeLibraryCount + 1) * sizeof(char *));
            nativeLibraryPaths[nativeLibraryCount++] = realpath(full, NULL);
            if (!nativeLibraryPaths[nativeLibraryCount - 1]) exitWithError("Cannot resolve library '%s'", full);
        }
        free(full);
    }

    closedir(dir);
}

#endif
void loadNativeLibraries(const char *path) {
#ifdef _WIN32
    loadWindows(path);
#else
    loadUnix(path);
#endif
}
