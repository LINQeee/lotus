cmake_minimum_required(VERSION 3.25)
project(compiler C)

set(CMAKE_C_STANDARD 11)

set(LLVM_DIR "/opt/homebrew/opt/llvm/lib/cmake/llvm")

find_package(LLVM REQUIRED CONFIG)

message(STATUS "LLVM version: ${LLVM_VERSION}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(compiler
        src/main.c
        src/lexer/lexer.c
        src/llvm/llvm.c
        src/parser/parser.c
        src/parser/parser.h
        src/utils/utils.c
        src/llvm/utils/llvm_extension.h
        src/llvm/utils/llvm_extension.c
        src/llvm/generator/generator.c
        src/llvm/generator/generator.h
        src/llvm/generator/statement/statement_generator.h
        src/llvm/generator/statement/statement_generator.c
        src/llvm/generator/expression/expression_generator.h
        src/llvm/generator/expression/expression_generator.c
        src/llvm/var_manager.c
        src/llvm/var_manager.h
        src/llvm/generator/expression/native_functions.c
        src/llvm/generator/expression/native_functions.h
        src/llvm/generator/expression/functions.c
        src/llvm/generator/expression/functions.h
        src/native_loader/native_loader.h
        src/native_loader/native_loader.c
        src/parser/parser_utils.c
        src/parser/parser_utils.h
        src/utils/cutils.c
        src/utils/cutils.h
        src/parser/parser_expressions.h
        src/parser/parser_statements.h
        src/parser/parser_statements.c
        src/parser/parser_expressions.c
        src/llvm/lotus_types.h
        src/type_checker/diagnostics.c
        src/type_checker/diagnostics.h
        src/type_checker/type_checker.c
        src/type_checker/type_checker.h
        src/type_checker/type_rules.c
        src/llvm/lotus_utils.c
        src/type_checker/type_rules.h
        src/type_checker/type_scope.c
        src/type_checker/type_scope.h
        src/llvm/llvm.h
)

target_link_libraries(compiler
        LLVM
)