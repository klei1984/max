set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_COMPILER gcc -m32)
set(CMAKE_CXX_COMPILER g++ -m32)

set(CMAKE_SKIP_RPATH FALSE)

set(CONFIGURE_EXTRA_ARGS
	--build=i686-pc-linux-gnu
	CC=gcc
	CXX=g++
	CFLAGS=-m32
	CXXFLAGS=-m32
	LDFLAGS=-m32
)

# Every -Wno-* comes after -Wall. gcc lets an explicit -Wno-X win wherever it
# sits, but the clang toolchains genuinely need this order (there, a -Wno-X
# before -Wall is silently re-enabled by it), so both families keep the same
# layout and the flag list stays safe to copy between them.
add_compile_options(
	-m32
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
