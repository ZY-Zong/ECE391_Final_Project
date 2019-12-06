# This file exists only to support CLion functions, not for actual compile.

cmake_minimum_required(VERSION 3.10)

include(tools/toolchain_i386_elf_gcc.cmake)

project(AuroraOS C ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fno-builtin -fno-stack-protector -nostdlib -Wno-builtin-declaration-mismatch -Wno-implicit-int")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib -static -Ttext=0x400000")

set(CMAKE_VERBOSE_MAKEFILE OFF)  # Enable this if you want to see verbose log

file(GLOB_RECURSE OS_C_SRC  student-distrib/*.c)
file(GLOB_RECURSE OS_S_SRC  student-distrib/*.S)
list(REMOVE_ITEM  OS_S_SRC  student-distrib/boot.S)  # boot.S must be compiled and linked first
file(GLOB_RECURSE OS_HEADER student-distrib/*.h)

add_executable(AuroraOS student-distrib/boot.S ${OS_S_SRC} ${OS_C_SRC} ${OS_HEADER})

set(ELF_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/student-distrib)
set(ELF_OUTPUT_NAME bootimg)

set_target_properties(AuroraOS PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ELF_OUTPUT_PATH}
        OUTPUT_NAME ${ELF_OUTPUT_NAME})

# Generate
add_custom_command(TARGET AuroraOS POST_BUILD
        COMMENT "Generating MP3 img..."
        COMMAND cd ${PROJECT_SOURCE_DIR} && ${PROJECT_SOURCE_DIR}/debug_mac.sh)

