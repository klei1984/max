set(CMAKE_SYSTEM_NAME Windows)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

# Root of the MSYS2 installation. Override with the MSYS2_ROOT environment
# variable when MSYS2 is not installed at the default location (CI runners
# using msys2/setup-msys2 unpack it under the runner temp directory).
if(DEFINED ENV{MSYS2_ROOT})
	file(TO_CMAKE_PATH "$ENV{MSYS2_ROOT}" MSYS2_ROOT)
else()
	set(MSYS2_ROOT c:/msys64)
endif()

# UCRT64 links against Microsoft's Universal CRT rather than the legacy
# msvcrt.dll used by the mingw64 prefix, and is the preferred 64 bit Windows
# toolchain for this project.
set(PREFIX ${MSYS2_ROOT}/ucrt64)
set(TOOLSET "x86_64-w64-mingw32")

set(CMAKE_FIND_ROOT_PATH ${PREFIX}/${TOOLSET})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# HINTS is searched ahead of the system PATH, so the compiler is taken from
# this toolchain's own prefix regardless of which MSYS2 shell drives the
# build. Without it a ucrt64 build started from a mingw64 shell would
# silently pick up the mingw64 compiler.
find_program(CMAKE_RC_COMPILER NAMES windres HINTS ${PREFIX}/bin REQUIRED)
find_program(CMAKE_C_COMPILER NAMES ${TOOLSET}-gcc HINTS ${PREFIX}/bin REQUIRED)
find_program(CMAKE_CXX_COMPILER NAMES ${TOOLSET}-g++ HINTS ${PREFIX}/bin REQUIRED)

set(CONFIGURE_EXTRA_ARGS
	--target=${TOOLSET}
	--host=${TOOLSET}
	--build=${TOOLSET}
	CC=${CMAKE_C_COMPILER}
	CXX=${CMAKE_CXX_COMPILER}
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
