cmake_minimum_required (VERSION 3.10)

project(max)
set(GAME_BINARY "maxrun")

set(GAME_VERSION_MAJOR "0")
set(GAME_VERSION_MINOR "0")
set(GAME_VERSION_PATCH "0")
set(GAME_VERSION "${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_PATCH}")

enable_language(C ASM)

set(GAME_INCLUDES "")
set(GAME_SOURCES "")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/")

find_package(PythonInterp)

set(CHGC_TOOL "${CMAKE_CURRENT_SOURCE_DIR}/util/chgcalls.py")
set(CHGC_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/conf/wrappers.conf")
set(CHGC_RENAME_FILE "${CMAKE_CURRENT_SOURCE_DIR}/conf/fnmap_chgcalls.conf")

set(GAME_SOURCES
    ${GAME_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/${GAME_BINARY}.S
)

add_custom_command(
    COMMAND ${PYTHON_EXECUTABLE} ${CHGC_TOOL} --rename-config=${CHGC_RENAME_FILE} ${CHGC_CONFIG_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/res/${GAME_BINARY}.in ${CMAKE_CURRENT_BINARY_DIR}/${GAME_BINARY}.S
    DEPENDS ${CHGC_TOOL} ${CHGC_CONFIG_FILE} ${CHGC_RENAME_FILE}
    OUTPUT ${GAME_BINARY}.S
    COMMENT "Generating ${GAME_BINARY}.S."
)

set(MKW_TOOL "${CMAKE_CURRENT_SOURCE_DIR}/util/mkwrappers.py")
set(MKW_RENAME_FILE "${CMAKE_CURRENT_SOURCE_DIR}/conf/fnmap_posix.conf")
set(MKW_FLAGS "")

set(GAME_SOURCES
    ${GAME_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/wrappers.S
)

add_custom_command(
    COMMAND ${PYTHON_EXECUTABLE} ${MKW_TOOL} ${MKW_FLAGS} -o ${CMAKE_CURRENT_BINARY_DIR}/wrappers.S -r ${MKW_RENAME_FILE} ${CHGC_CONFIG_FILE}
    DEPENDS ${MKW_TOOL} ${CHGC_CONFIG_FILE} ${MKW_RENAME_FILE}
    OUTPUT wrappers.S
    COMMENT "Generating wrappers.S."
)

find_package(SDL2 REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GAME_INCLUDES}
    ${SDL2_INCLUDE_DIRS}
)

add_executable(${GAME_BINARY} ${GAME_SOURCES})
set_target_properties(${GAME_BINARY} PROPERTIES COMPILE_FLAGS "-m32 -O0 -no-pie" LINK_FLAGS "-m32 -O0 -no-pie")
target_link_libraries(${GAME_BINARY} ${SDL2_LIBRARIES})
