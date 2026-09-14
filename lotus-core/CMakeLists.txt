cmake_minimum_required(VERSION 4.2)
project(lotus_core C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../libs")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../libs")
add_library(lotus_core SHARED library.c
        lotus.h)
