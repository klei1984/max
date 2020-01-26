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

set(CODEGEN_TOOL "${CMAKE_CURRENT_SOURCE_DIR}/util/codegen.py")
set(CODEGEN_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/conf/codegen.json")

if (WIN32)
	set(CODEGEN_OPTIONS "--underscore")
endif (WIN32)

set(GAME_SOURCES
    ${GAME_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/${GAME_BINARY}.S
)

add_custom_command(
    COMMAND ${PYTHON_EXECUTABLE} ${CODEGEN_TOOL} ${CODEGEN_OPTIONS} --input=${CMAKE_CURRENT_SOURCE_DIR}/res/${GAME_BINARY}.in --output=${CMAKE_CURRENT_BINARY_DIR} ${CODEGEN_CONFIG_FILE}
    DEPENDS ${CODEGEN_TOOL} ${CODEGEN_CONFIG_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/res/${GAME_BINARY}.in
    OUTPUT ${GAME_BINARY}.S ${GAME_BINARY}.h wrappers.h
    COMMENT "Generating ${GAME_BINARY}.S ${GAME_BINARY}.h wrappers.h."
)

find_package(SDL2 REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}
    ${GAME_INCLUDES}
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS_HACK}
)

add_executable(${GAME_BINARY} ${GAME_SOURCES})
set_target_properties(${GAME_BINARY} PROPERTIES COMPILE_FLAGS "-m32 -O0 -no-pie" LINK_FLAGS "-m32 -O0 -no-pie")
target_link_libraries(${GAME_BINARY} ${SDL2_LIBRARIES})
