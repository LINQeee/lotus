cmake_minimum_required(VERSION 3.25)
project(compiler C CXX)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Lotus currently requires a 64-bit platform")
endif()

set(CMAKE_C_STANDARD 11)

find_program(LLVM_CONFIG NAMES llvm-config llvm-config-22 HINTS /opt/homebrew/opt/llvm/bin)
if(LLVM_CONFIG AND NOT LLVM_DIR)
    execute_process(COMMAND ${LLVM_CONFIG} --cmakedir OUTPUT_VARIABLE LLVM_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

find_package(LLVM REQUIRED CONFIG)
if(LLVM_PACKAGE_VERSION VERSION_LESS 22)
    message(FATAL_ERROR "LLVM 22 or newer is required")
endif()

message(STATUS "LLVM version: ${LLVM_VERSION}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(compiler
        src/main.c
        src/cli/project.c
        src/llvm/executable.c
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
        src/llvm/generator/expression/native_caller.c
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

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS templates/main.lt)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/templates/main.lt" PROJECT_TEMPLATE_BYTES HEX)
string(REGEX REPLACE "(..)" "0x\\1," PROJECT_TEMPLATE_BYTES "${PROJECT_TEMPLATE_BYTES}")
configure_file(src/cli/project_template.h.in generated/project_template.h @ONLY)
target_include_directories(compiler PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

if(TARGET LLVM)
    target_link_libraries(compiler PRIVATE LLVM)
else()
    llvm_map_components_to_libnames(LLVM_LIBS core executionengine mcjit native passes)
    target_link_libraries(compiler PRIVATE ${LLVM_LIBS})
    set_target_properties(compiler PROPERTIES LINKER_LANGUAGE CXX)
endif()
target_link_libraries(compiler PRIVATE ${CMAKE_DL_LIBS})
if(WIN32)
    enable_language(RC)
    target_include_directories(compiler PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_sources(compiler PRIVATE src/lotus.rc)
endif()
option(LOTUS_BUILD_CORE "Build and install the standard runtime" ON)
if(LOTUS_BUILD_CORE)
    add_subdirectory(lotus-core)
    add_dependencies(compiler lotus_core)
endif()
set_target_properties(compiler PROPERTIES OUTPUT_NAME lotus)
set(LOTUS_RUNTIME_DIR "${CMAKE_SOURCE_DIR}/libs" CACHE PATH "Default native library directory")
target_compile_definitions(compiler PRIVATE LOTUS_RUNTIME_DIR="${LOTUS_RUNTIME_DIR}")
install(TARGETS compiler RUNTIME DESTINATION bin)
