set(CMAKE_SYSTEM_NAME Windows)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PREFIX /usr)
set(TOOLSET "i686-w64-mingw32")

set(MAX_BUILD_TESTS OFF)

set(CMAKE_FIND_ROOT_PATH ${PREFIX}/${TOOLSET})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

find_program(CMAKE_RC_COMPILER NAMES ${TOOLSET}-windres REQUIRED)
find_program(CMAKE_C_COMPILER NAMES ${TOOLSET}-gcc REQUIRED)
find_program(CMAKE_CXX_COMPILER NAMES ${TOOLSET}-g++ REQUIRED)

set(CONFIGURE_EXTRA_ARGS
	--target=${TOOLSET}
	--host=${TOOLSET}
	--build=x86_64-linux
	CC=${CMAKE_C_COMPILER}
	CXX=${CMAKE_CXX_COMPILER}
	CFLAGS=-m32
	CXXFLAGS=-m32
	LDFLAGS=-m32
)

add_compile_options(
	-DCROSS
	-m32
	$<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder>
	-Wall
	-Wno-switch
	-Wno-unused-function
	-Wno-unused-variable
	-Wignored-qualifiers
	-Wshadow=local
	-Wtype-limits
	-Wlogical-op
	-fno-eliminate-unused-debug-types
)