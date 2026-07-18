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

# CLANG64 is a UCRT based prefix built around clang/LLVM (libc++, compiler-rt,
# lld) rather than the GCC runtime used by ucrt64.
set(PREFIX ${MSYS2_ROOT}/clang64)
set(TOOLSET "x86_64-w64-mingw32")

# Unlike the gcc prefixes, clang64 has no ${TOOLSET} sysroot subdirectory --
# its libraries and headers sit directly under ${PREFIX}/lib and
# ${PREFIX}/include, so the find root is the prefix itself. Pointing it at
# ${PREFIX}/${TOOLSET} would name a directory that does not exist and make
# every find_library/find_path call fail.
set(CMAKE_FIND_ROOT_PATH ${PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# HINTS is searched ahead of the system PATH. `clang` is an unqualified name
# present in several MSYS2 prefixes, so without the hint the build would pick
# up whichever clang happens to come first on PATH rather than this one.
find_program(CMAKE_RC_COMPILER NAMES llvm-windres HINTS ${PREFIX}/bin REQUIRED)
find_program(CMAKE_C_COMPILER NAMES clang HINTS ${PREFIX}/bin REQUIRED)
find_program(CMAKE_CXX_COMPILER NAMES clang++ HINTS ${PREFIX}/bin REQUIRED)

set(CONFIGURE_EXTRA_ARGS
	--target=${TOOLSET}
	--host=${TOOLSET}
	--build=${TOOLSET}
	CC=${CMAKE_C_COMPILER}
	CXX=${CMAKE_CXX_COMPILER}
)

# Flag order matters for clang: unlike gcc, which lets an explicit -Wno-X win
# regardless of position, clang applies -W options left to right, so a -Wno-X
# placed before -Wall is silently re-enabled by it. Every -Wno-* must therefore
# come after -Wall.
#
# -Wshadow is deliberately absent. The gcc toolchains use -Wshadow=local, which
# warns only when a local shadows another local or a parameter. Clang has no
# equivalent narrowing (-Wshadow-local does not exist, and -Wno-shadow-field /
# -Wno-shadow-uncaptured-local do not suppress it): its -Wshadow always also
# flags locals shadowing fields and globals, which this codebase does
# idiomatically. The narrow case stays covered by the gcc toolchains.
add_compile_options(
	-Wall
	$<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder-ctor>
	-Wno-switch
	-Wno-unused-function
	-Wno-unused-variable
	-Wignored-qualifiers
	-Wno-deprecated-declarations
)
