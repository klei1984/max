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

add_compile_options(
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
