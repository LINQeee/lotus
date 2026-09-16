#include <stdio.h>
#include <stdlib.h>

#include "lexer/lexer.h"
#include "native_loader/native_loader.h"
#include "parser/parser.h"
#include "type_checker/type_checker.h"
#include "utils/utils.h"
#include "llvm/llvm.h"

char *readScript(const char *filePath) {
    FILE *file = fopen(filePath, "r");

    fseek(file, 0, SEEK_END);
    const long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *fileContents = malloc(fileSize + 1);

    const size_t bytesRead = fread(fileContents, 1, fileSize, file);
    fileContents[bytesRead] = '\0';
    fclose(file);
    return fileContents;
}

int main(void) {
    const char *source = readScript("../test.lt");

    Token *tokens = tokenize(source, "test.lt");

    ProgramNode *program = buildAST(tokens);

    initCodegen();
    loadNativeLibraries("../libs");

    TypeChecker *checker = typeCheckerCreate(source);

    typeCheckerCheckProgram(checker, program);
    typeCheckerDestroy(checker);

    printAST(program);

    codegenProgram(program);
    return 0;
}
