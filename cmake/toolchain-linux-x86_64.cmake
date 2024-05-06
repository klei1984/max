set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_SKIP_RPATH FALSE)

set(CONFIGURE_EXTRA_ARGS --build=x86_64-linux-gnu CC=gcc CXX=g++)

set(MAX_CFLAGS "-Wcomments -Wignored-qualifiers -Wshadow=local -Wtype-limits -Wparentheses -Wuninitialized -Wmaybe-uninitialized -Wformat -Wformat-overflow -Wformat-truncation -Wlogical-op -Wlogical-not-parentheses -fno-eliminate-unused-debug-types")
