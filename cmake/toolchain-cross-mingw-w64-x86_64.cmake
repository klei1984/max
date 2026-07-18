set(CMAKE_SYSTEM_NAME Windows)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PREFIX /usr)
set(TOOLSET "x86_64-w64-mingw32")

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
)

# Every -Wno-* comes after -Wall. gcc lets an explicit -Wno-X win wherever it
# sits, but the clang toolchains genuinely need this order (there, a -Wno-X
# before -Wall is silently re-enabled by it), so both families keep the same
# layout and the flag list stays safe to copy between them.
add_compile_options(
	-DCROSS
	-Wall
	$<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder>
	-Wno-switch
	-Wno-unused-function
	-Wno-unused-variable
	-Wignored-qualifiers
	-Wshadow=local
	-Wtype-limits
	-Wvla
	-Wlogical-op
	-fno-eliminate-unused-debug-types
)