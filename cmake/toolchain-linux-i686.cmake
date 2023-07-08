set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_COMPILER gcc -m32)
set(CMAKE_CXX_COMPILER g++ -m32)

set(CONFIGURE_EXTRA_ARGS --build=i686-pc-linux-gnu CC=gcc CXX=g++ CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32)

set(MAX_CFLAGS "-Wcomments -Wignored-qualifiers -Wshadow=local -Wtype-limits -Wparentheses -Wuninitialized -Wmaybe-uninitialized -Wformat -Wformat-overflow -Wformat-truncation -Wlogical-op -Wlogical-not-parentheses -fno-eliminate-unused-debug-types")
