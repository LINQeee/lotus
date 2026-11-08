#include "executable.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <spawn.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Core.h>

#include "llvm.h"
#include "utils/llvm_extension.h"
#include "../native_loader/native_loader.h"
#include "../utils/cutils.h"

extern char **environ;

static int runProcess(char **args) {
    fflush(NULL);
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
    char **args = mallocSafe((nativeLibraryCount * 5 + 5) * sizeof(char *));
    char **directories = mallocSafe((nativeLibraryCount + 1) * sizeof(char *));
    args[0] = (char *)(cc ? cc : "cc");
    args[1] = object;
    args[2] = "-o";
    args[3] = (char *)outputBuildPath;
    size_t count = 4;
    for (size_t i = 0; i < nativeLibraryCount; i++) {
        char *directory = mallocSafe(strlen(nativeLibraryPaths[i]) + 1);
        strcpy(directory, nativeLibraryPaths[i]);
        *strrchr(directory, '/') = '\0';
        directories[i] = directory;
        args[count++] = nativeLibraryPaths[i];
        args[count++] = "-Xlinker";
        args[count++] = "-rpath";
        args[count++] = "-Xlinker";
        args[count++] = directory;
    }
    args[count] = NULL;
    int result = runProcess(args);
    for (size_t i = 0; i < nativeLibraryCount; i++) free(directories[i]);
    free(directories);
    free(args);
    return result;
}

int buildExecutable(const char *outputBuildPath) {
    char object[] = "/tmp/lotus-object-XXXXXX";
    int fd = mkstemp(object);
    if (fd < 0) exitWithError("Cannot create object file: %s", strerror(errno));
    close(fd);
    int result = emitObject(object);
    if (!result) result = linkExecutable(object, outputBuildPath);
    unlink(object);
    return result;
}
