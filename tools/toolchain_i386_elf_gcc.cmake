# This file includes configurations for cross compiling for i386 with no stdlib.
# liuzikai 2019-12-05

# Usage: `include(tools/toolchain_i386_elf_gcc.cmake)` in CMakeLists.txt, before project() command
# Notice: Currently only support C and ASM with no stdlib

# CMake system configs. These configs affects the way to pass flags.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i386)

# ---------- Toolchain Configurations ----------

# The following lines set the toolchain. Setting toolchain in CLion preference is also valid.

set(TOOLCHAIN_PREFIX i386-elf-)
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc    )
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc    )
#set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++    )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib")

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
set(CMAKE_THREAD_LIBS_INIT)

