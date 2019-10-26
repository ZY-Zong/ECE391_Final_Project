# This file exists only to support CLion functions, not for actual compile.

cmake_minimum_required(VERSION 3.10)

project(UnnamedOS)

file(GLOB_RECURSE ALL_C_FILES *.c)
file(GLOB_RECURSE ALL_S_FILES *.S)
file(GLOB_RECURSE ALL_HEADER_FILES *.h)
add_executable(ALL_FILES ${ALL_C_FILES} ${ALL_S_FILES} ${ALL_HEADER_FILES} student-distrib/terminal.c student-distrib/terminal.h)
add_custom_target(NULL)