#include "executable.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <process.h>
#include <stdint.h>
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Core.h>

#include "llvm.h"
#include "utils/llvm_extension.h"
#include "../native_loader/native_loader.h"
#include "../utils/cutils.h"

#ifndef _WIN32
extern char **environ;
#else
static char *quoteArgument(const char *arg) {
    char *quoted = mallocSafe(strlen(arg) * 2 + 3);
    char *out = quoted;
    *out++ = '"';
    while (*arg) {
        size_t slashes = 0;
        while (*arg == '\\') {
            slashes++;
            arg++;
        }
        size_t count = (*arg == '"' || !*arg) ? slashes * 2 : slashes;
        while (count--) *out++ = '\\';
        if (*arg == '"') *out++ = '\\';
        if (*arg) *out++ = *arg++;
    }
    *out++ = '"';
    *out = '\0';
    return quoted;
}
#endif

static int runProcess(char **args) {
    fflush(NULL);
#ifdef _WIN32
    size_t count = 0;
    while (args[count]) count++;
    char **quoted = mallocSafe((count + 1) * sizeof(char *));
    for (size_t i = 0; i < count; i++) quoted[i] = quoteArgument(args[i]);
    quoted[count] = NULL;
    intptr_t result = _spawnvp(_P_WAIT, args[0], (const char *const *)quoted);
    for (size_t i = 0; i < count; i++) free(quoted[i]);
    free(quoted);
    if (result == -1) {
        fprintf(stderr, "Cannot execute '%s': %s\n", args[0], strerror(errno));
        return 1;
    }
    return (int)result;
#else
    pid_t pid;
    int error = posix_spawnp(&pid, args[0], NULL, NULL, args, environ);
    if (error) {
        fprintf(stderr, "Cannot execute '%s': %s\n", args[0], strerror(error));
        return 1;
    }
    int status;
    while (waitpid(pid, &status, 0) < 0)
        if (errno != EINTR) exitWithError("Cannot wait for '%s': %s", args[0], strerror(errno));
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
#endif
}

static int emitObject(char *object) {
    LLVMTargetMachineRef machine = getTargetMachine();
    char *error = NULL;
    int result = LLVMTargetMachineEmitToFile(machine, cg->module, object, LLVMObjectFile, &error);
    LLVMDisposeTargetMachine(machine);
    if (result) {
        fprintf(stderr, "Cannot emit object: %s\n", error);
        LLVMDisposeMessage(error);
    }
    return result;
}

static int linkExecutable(char *object, const char *outputBuildPath) {
    const char *cc = getenv("CC");
    char **args = mallocSafe((nativeLibraryCount * 5 + 6) * sizeof(char *));
#ifndef _WIN32
    char **directories = mallocSafe((nativeLibraryCount + 1) * sizeof(char *));
#endif
#ifdef _WIN32
    args[0] = (char *)(cc ? cc : "clang");
#else
    args[0] = (char *)(cc ? cc : "cc");
#endif
    args[1] = object;
    args[2] = "-o";
    args[3] = (char *)outputBuildPath;
    size_t count = 4;
    for (size_t i = 0; i < nativeLibraryCount; i++) {
        args[count++] = nativeLibraryPaths[i];
#ifndef _WIN32
        char *directory = mallocSafe(strlen(nativeLibraryPaths[i]) + 1);
        strcpy(directory, nativeLibraryPaths[i]);
        *strrchr(directory, '/') = '\0';
        directories[i] = directory;
        args[count++] = "-Xlinker";
        args[count++] = "-rpath";
        args[count++] = "-Xlinker";
        args[count++] = directory;
#endif
    }
#ifndef _WIN32
    args[count++] = "-lm";
#endif
    args[count] = NULL;
    int result = runProcess(args);
#ifndef _WIN32
    for (size_t i = 0; i < nativeLibraryCount; i++) free(directories[i]);
    free(directories);
#endif
    free(args);
    return result;
}

#ifdef _WIN32
static void copyRuntime(const char *outputBuildPath) {
    char *output = _fullpath(NULL, outputBuildPath, 0);
    if (!output) exitWithError("Cannot resolve output '%s'", outputBuildPath);
    char *name = strrchr(output, '\\');
    name[1] = '\0';
    for (size_t i = 0; i < nativeLibraryCount; i++) {
        const char *source = nativeLibraryPaths[i];
        const char *file = strrchr(source, '\\') + 1;
        char *destination = mallocSafe(strlen(output) + strlen(file) + 1);
        sprintf(destination, "%s%s", output, file);
        if (_stricmp(source, destination) && !CopyFileA(source, destination, FALSE))
            exitWithError("Cannot copy runtime to '%s': Windows error %lu", destination, GetLastError());
        free(destination);
    }
    free(output);
}
#endif

int buildExecutable(const char *outputBuildPath) {
#ifdef _WIN32
    char temp[MAX_PATH], object[MAX_PATH];
    DWORD length = GetTempPathA(sizeof(temp), temp);
    if (!length || length >= sizeof(temp) || !GetTempFileNameA(temp, "lot", 0, object))
        exitWithError("Cannot create object file: Windows error %lu", GetLastError());
#else
    char object[] = "/tmp/lotus-object-XXXXXX";
    int fd = mkstemp(object);
    if (fd < 0) exitWithError("Cannot create object file: %s", strerror(errno));
    close(fd);
#endif
    int result = emitObject(object);
    if (!result) result = linkExecutable(object, outputBuildPath);
    remove(object);
#ifdef _WIN32
    if (!result) copyRuntime(outputBuildPath);
#endif
    return result;
}
