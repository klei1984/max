set(CMAKE_SYSTEM_NAME Windows)

set(PREFIX /usr/local/win32)
set(TOOLSET "i686-w64-mingw32")

set(MAX_BUILD_TESTS OFF)

set(CMAKE_FIND_ROOT_PATH ${PREFIX}/${TOOLSET})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

find_program(CMAKE_RC_COMPILER NAMES ${TOOLSET}-windres)
find_program(CMAKE_C_COMPILER NAMES ${TOOLSET}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${TOOLSET}-g++)

set(CONFIGURE_EXTRA_ARGS --target=${TOOLSET} --host=${TOOLSET} --build=x86_64-linux CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32)

add_compile_options(-m32)
