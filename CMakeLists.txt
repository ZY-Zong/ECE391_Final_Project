# This file exists only to support CLion functions, not for actual compile.

cmake_minimum_required(VERSION 3.10)

project(AuroraOS)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i386)

file(GLOB_RECURSE ALL_C_FILES *.c */*.c)
file(GLOB_RECURSE ALL_S_FILES *.S */*.S)
file(GLOB_RECURSE ALL_HEADER_FILES *.h */*.h)
add_executable(ALL_FILES ${ALL_C_FILES} ${ALL_S_FILES} ${ALL_HEADER_FILES} student-distrib/terminal.c student-distrib/terminal.h student-distrib/modex.c student-distrib/modex.h student-distrib/text.c student-distrib/text.h)
add_custom_target(NULL)