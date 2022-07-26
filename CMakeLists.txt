cmake_minimum_required (VERSION 3.10)

project(max)
set(GAME_BINARY "maxrun")

find_package(Git)

if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --always
        RESULT_VARIABLE GIT_RESULT
        OUTPUT_VARIABLE GIT_OUTPUT
        ERROR_QUIET
    )

    string(STRIP "${GIT_OUTPUT}" GIT_OUTPUT)

    string(REGEX MATCH "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)\\-([0-9]+)\\-([a-fA-F0-9]+)$" GIT_MATCHES "${GIT_OUTPUT}")
    if(CMAKE_MATCH_COUNT EQUAL 5)
        set(GAME_VERSION_MAJOR ${CMAKE_MATCH_1})
        set(GAME_VERSION_MINOR ${CMAKE_MATCH_2})
        set(GAME_VERSION_PATCH ${CMAKE_MATCH_3})
        set(GAME_VERSION_BUILD ${CMAKE_MATCH_4})
        set(GAME_VERSION_REVISION ${CMAKE_MATCH_5})
        set(GAME_VERSION "${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_PATCH} Build ${GAME_VERSION_BUILD} Revision ${GAME_VERSION_REVISION}")
    else()
        string(REGEX MATCH "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)$" GIT_MATCHES "${GIT_OUTPUT}")
        if(CMAKE_MATCH_COUNT EQUAL 3)
            set(GAME_VERSION_MAJOR ${CMAKE_MATCH_1})
            set(GAME_VERSION_MINOR ${CMAKE_MATCH_2})
            set(GAME_VERSION_PATCH ${CMAKE_MATCH_3})
            set(GAME_VERSION "${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_PATCH}")
        else()
            string(REGEX MATCH "^([a-fA-F0-9]+)$" GIT_MATCHES "${GIT_OUTPUT}")
            if(CMAKE_MATCH_COUNT EQUAL 1)
                set(GAME_VERSION_REVISION ${CMAKE_MATCH_1})
            else()
                string(TIMESTAMP DATE_TIME "%Y-%m-%d %H:%M:%S")
                set(GAME_VERSION_REVISION "${DATE_TIME}")
            endif()

            set(GAME_VERSION_MAJOR "0")
            set(GAME_VERSION_MINOR "2")
            set(GAME_VERSION_PATCH "0")
            set(GAME_VERSION "${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_PATCH} Revision ${GAME_VERSION_REVISION}")
        endif()
    endif()
else()
    set(GAME_VERSION_MAJOR "0")
    set(GAME_VERSION_MINOR "2")
    set(GAME_VERSION_PATCH "0")
    set(GAME_VERSION "${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_PATCH}")
endif()

message(STATUS "Set build version to v${GAME_VERSION}")

enable_language(C ASM)

set(GAME_INCLUDES "")
set(GAME_SOURCES "")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/")

find_package(PythonInterp)

set(CODEGEN_TOOL "${CMAKE_CURRENT_SOURCE_DIR}/util/codegen.py")
set(CODEGEN_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/conf/codegen.json")

if (MINGW)
    set(CODEGEN_OPTIONS "--underscore")
    add_definitions(-DSDL_MAIN_HANDLED)
    add_compile_options(-mno-ms-bitfields)
    add_link_options(-mconsole)
endif (MINGW)

if (UNIX)
    set(ENV{PKG_CONFIG_PATH} "/usr/lib/i386-linux-gnu/pkgconfig")
endif (UNIX)

set(GAME_SOURCES
    ${GAME_SOURCES}
#    ${CMAKE_CURRENT_BINARY_DIR}/${GAME_BINARY}.S
)

set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/${GAME_BINARY}.S
    PROPERTIES
    OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${GAME_BINARY}.h;${CMAKE_CURRENT_SOURCE_DIR}/src/user.h"
)

add_custom_command(
    COMMAND ${PYTHON_EXECUTABLE} ${CODEGEN_TOOL} ${CODEGEN_OPTIONS} --input=${CMAKE_CURRENT_SOURCE_DIR}/res/${GAME_BINARY}.in --output=${CMAKE_CURRENT_BINARY_DIR} ${CODEGEN_CONFIG_FILE}
    DEPENDS ${CODEGEN_TOOL} ${CODEGEN_CONFIG_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/res/${GAME_BINARY}.in
    OUTPUT ${GAME_BINARY}.S ${GAME_BINARY}.h wrappers.h
    COMMENT "Generating ${GAME_BINARY}.S ${GAME_BINARY}.h wrappers.h."
)

INCLUDE(FindPkgConfig)

PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2 )
PKG_SEARCH_MODULE(SDL2_MIXER REQUIRED SDL2_mixer)
PKG_SEARCH_MODULE(SDL2_NET REQUIRED SDL2_net>=2.0.0)

if(NOT TARGET SDL2::SDL2)
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    set_property(TARGET SDL2::SDL2 APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${SDL2_LIBRARIES})
    set_property(TARGET SDL2::SDL2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIRS})
endif()

if(NOT TARGET SDL2::NET)
    add_library(SDL2::NET INTERFACE IMPORTED)
    set_property(TARGET SDL2::NET APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${SDL2_NET_LIBRARIES})
    set_property(TARGET SDL2::NET PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SDL2_NET_INCLUDE_DIRS})
endif()

if(NOT TARGET SDL2::MIXER)
    add_library(SDL2::MIXER INTERFACE IMPORTED)
    set_property(TARGET SDL2::MIXER APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${SDL2_MIXER_LIBRARIES})
    set_property(TARGET SDL2::MIXER PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SDL2_MIXER_INCLUDE_DIRS})
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}
    ${GAME_INCLUDES}
)

add_executable(${GAME_BINARY} ${GAME_SOURCES})
set_target_properties(${GAME_BINARY} PROPERTIES COMPILE_FLAGS "-Wtype-limits -Wparentheses -Wuninitialized -Wmaybe-uninitialized -Wformat -m32 -O0 -fno-eliminate-unused-debug-types -no-pie -DGAME_VERSION=\"\\\"v${GAME_VERSION}\\\"\"" LINK_FLAGS "-m32 -O0 -no-pie")
target_link_libraries(${GAME_BINARY} SDL2::SDL2 SDL2::NET SDL2::MIXER m)

set(CPACK_PACKAGE_VERSION_MAJOR ${GAME_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${GAME_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${GAME_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})

set(CPACK_PACKAGE_NAME "max-port")
set(CPACK_PACKAGE_CONTACT "M.A.X. Port Team")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "M.A.X. Port v${GAME_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION "M.A.X. Port is an SDL library based runtime executable for the 1996 DOS game M.A.X.: Mechanized Assault & Exploration developed and published by Interplay Productions.")

install(FILES "LICENSE" DESTINATION .)

if(UNIX)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_SECTION "games")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://klei1984.github.io/max/")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")

    install(TARGETS ${GAME_BINARY}
        CONFIGURATIONS ${CMAKE_BUILD_TYPE}
        RUNTIME DESTINATION .
    )
endif()

if(MINGW)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "${GAME_BINARY}")
    set(CPACK_NSIS_DISPLAY_NAME "M.A.X. Port")
    set(CPACK_NSIS_PACKAGE_NAME "M.A.X. Port v${GAME_VERSION}")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://klei1984.github.io/max/")

    install(TARGETS ${GAME_BINARY}
        CONFIGURATIONS ${CMAKE_BUILD_TYPE}
        RUNTIME DESTINATION .
    )

    install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${GAME_BINARY}${CMAKE_EXECUTABLE_SUFFIX}\" \"\" \"\")
        "
    )
endif()

INCLUDE(CPack)
