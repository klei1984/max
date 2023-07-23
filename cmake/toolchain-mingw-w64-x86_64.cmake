set(CMAKE_SYSTEM_NAME Windows)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PREFIX c:/msys64/mingw64)
set(TOOLSET "x86_64-w64-mingw32")

set(CMAKE_FIND_ROOT_PATH ${PREFIX}/${TOOLSET})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

find_program(CMAKE_RC_COMPILER NAMES windres REQUIRED)
find_program(CMAKE_C_COMPILER NAMES ${TOOLSET}-gcc REQUIRED)
find_program(CMAKE_CXX_COMPILER NAMES ${TOOLSET}-g++ REQUIRED)

set(CONFIGURE_EXTRA_ARGS --target=${TOOLSET} --host=${TOOLSET} --build=${TOOLSET} CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER})

set(MAX_CFLAGS "-Wcomments -Wignored-qualifiers -Wshadow=local -Wtype-limits -Wparentheses -Wuninitialized -Wmaybe-uninitialized -Wformat -Wformat-overflow -Wformat-truncation -Wlogical-op -Wlogical-not-parentheses -fno-eliminate-unused-debug-types")
