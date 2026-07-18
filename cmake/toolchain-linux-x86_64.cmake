set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_SKIP_RPATH FALSE)

set(CONFIGURE_EXTRA_ARGS
	--build=x86_64-pc-linux-gnu
	CC=gcc
	CXX=g++
)

# Every -Wno-* comes after -Wall. gcc lets an explicit -Wno-X win wherever it
# sits, but the clang toolchains genuinely need this order (there, a -Wno-X
# before -Wall is silently re-enabled by it), so both families keep the same
# layout and the flag list stays safe to copy between them.
add_compile_options(
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
