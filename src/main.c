#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include "lexer/lexer.h"
#include "native_loader/native_loader.h"
#include "parser/parser.h"
#include "type_checker/type_checker.h"
#include "utils/utils.h"
#include "utils/cutils.h"
#include "llvm/llvm.h"

typedef struct {
    const char *file, *libs, *irPath;
    bool check, debug, build, help;
} Options;

static void printHelp(void) {
    puts("Usage: lotus [run|build|check] [options] FILE\n"
         "\n"
         "  run              Execute through JIT (default)\n"
         "  build            Build FILE.out without executing\n"
         "  check            Parse and type-check without execution\n"
         "  --libs DIR       Native library directory (overrides LOTUS_LIBS)\n"
         "  --debug          Print AST, IR and execution timing\n"
         "  --emit-ir FILE   Save optimized LLVM IR\n"
         "  -h, --help       Show this help");
}

static LLVMMemoryBufferRef readScript(const char *path) {
    LLVMMemoryBufferRef buffer;
    char *error = NULL;
    if (LLVMCreateMemoryBufferWithContentsOfFile(path, &buffer, &error))
        exitWithError("Cannot read '%s': %s", path, error);
    if (memchr(LLVMGetBufferStart(buffer), '\0', LLVMGetBufferSize(buffer)))
        exitWithError("Source '%s' contains a null byte", path);
    return buffer;
}

static Options parseArgs(int argc, char **argv) {
    Options options = {.libs = getenv("LOTUS_LIBS")};
    int first = 1;
    if (argc > 1 && (!strcmp(argv[1], "run") || !strcmp(argv[1], "build") || !strcmp(argv[1], "check"))) {
        options.check = !strcmp(argv[1], "check");
        options.build = !strcmp(argv[1], "build");
        first++;
    }
    for (int i = first; i < argc; i++) {
        const char *arg = argv[i];
        if (!strcmp(arg, "--help") || !strcmp(arg, "-h")) {
            options.help = true;
            return options;
        }
        if (!strcmp(arg, "--debug")) options.debug = true;
        else if (!strcmp(arg, "--libs") || !strcmp(arg, "--emit-ir")) {
            if (++i == argc) exitWithError("Missing value for %s", arg);
            if (!strcmp(arg, "--libs")) options.libs = argv[i];
            else options.irPath = argv[i];
        } else if (arg[0] == '-') exitWithError("Unknown option: %s", arg);
        else if (options.file) exitWithError("Expected one source file, got '%s'", arg);
        else options.file = arg;
    }
    if (options.check && options.irPath) exitWithError("--emit-ir requires run or build");
    return options;
}

int main(int argc, char **argv) {
    Options options = parseArgs(argc, argv);
    if (options.help || !options.file) {
        printHelp();
        return options.help ? 0 : 1;
    }
    LLVMMemoryBufferRef buffer = readScript(options.file);
    const char *source = LLVMGetBufferStart(buffer);
    Token *tokens = tokenize(source, options.file);
    ProgramNode *program = buildAST(tokens);
    initCodegen();
    loadNativeLibraries(options.libs ? options.libs : LOTUS_RUNTIME_DIR);
    TypeChecker *checker = typeCheckerCreate(source);
    typeCheckerCheckProgram(checker, program);
    typeCheckerDestroy(checker);
    if (options.debug) printAST(program);
    char *outputBuildPath = NULL;
    if (options.build) {
        outputBuildPath = mallocSafe(strlen(options.file) + 5);
        sprintf(outputBuildPath, "%s.out", options.file);
    }
    int result = options.check ? 0 : codegenProgram(program, options.debug, options.irPath, outputBuildPath);
    free(outputBuildPath);
    LLVMDisposeMemoryBuffer(buffer);
    return result;
}
