cmake_minimum_required (VERSION 3.24)

project(max LANGUAGES C CXX)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(setup)

set(GAME_INCLUDES "")
set(GAME_SOURCES "")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(${PROJECT_NAME} ${GAME_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${GAME_INCLUDES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD "20"
	COMPILE_FLAGS "-Wcomments -Wignored-qualifiers -Wshadow=local -Wtype-limits -Wparentheses -Wuninitialized -Wmaybe-uninitialized -Wformat -Wformat-overflow -Wformat-truncation -Wlogical-op -Wlogical-not-parentheses -fno-eliminate-unused-debug-types -DGAME_VERSION=\"\\\"v${GAME_VERSION}\\\"\""
)

if(NOT BUILD_SHARED_LIBS)
	set(${PROJECT_NAME}_deps SDL2::SDL2main Iconv::Iconv Freetype::Freetype SDL2::SDL2-static SDL2_net::SDL2_net-static SDL2_mixer::SDL2_mixer-static)

	target_link_options(${PROJECT_NAME} PUBLIC -static -static-libgcc -static-libstdc++)
	target_link_libraries(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_deps})
else()
	set(${PROJECT_NAME}_deps SDL2::SDL2main Iconv::Iconv Freetype::Freetype SDL2::SDL2 SDL2_net::SDL2_net SDL2_mixer::SDL2_mixer)

	target_link_libraries(${PROJECT_NAME} PRIVATE  ${${PROJECT_NAME}_deps})
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cmake/lib-iconv ${CMAKE_CURRENT_BINARY_DIR}/_deps/iconv-build)

include(sdl2)
include(sdl2_mixer)
include(sdl2_net)
include(freetype)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_OBJCOPY} ARGS --only-keep-debug	 $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.debug
	COMMAND ${CMAKE_STRIP} ARGS --strip-debug --strip-unneeded $<TARGET_FILE:${PROJECT_NAME}>
	COMMAND ${CMAKE_OBJCOPY} ARGS --add-gnu-debuglink ${PROJECT_NAME}.debug $<TARGET_FILE:${PROJECT_NAME}>
)

set(CPACK_PACKAGE_VERSION_MAJOR ${GAME_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${GAME_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${GAME_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})

set(CPACK_PACKAGE_NAME "max-port")
set(CPACK_PACKAGE_CONTACT "M.A.X. Port Team")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "M.A.X. Port v${GAME_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION "M.A.X. Port is an SDL library based runtime executable for the 1996 MS-DOS game M.A.X.: Mechanized Assault & Exploration developed and published by Interplay Productions.")
set(CPACK_SOURCE_IGNORE_FILES "/\.git/;/\.github/;/\.settings/;\.gitignore;.\.launch;\.project;\.cproject;\.clang-format;\.gdbinit;README.md;/doc/;${CMAKE_BINARY_DIR};/dependencies/.*\.tar\.gz")

install(FILES "LICENSE" DESTINATION .)
install(FILES "assets/PATCHES.RES" DESTINATION .)
install(FILES "assets/settings.ini" DESTINATION .)
install(FILES "assets/lang_english.ini" DESTINATION .)
install(FILES "assets/lang_french.ini" DESTINATION .)
install(FILES "assets/lang_german.ini" DESTINATION .)
install(FILES "assets/lang_italian.ini" DESTINATION .)
install(FILES "assets/lang_spanish.ini" DESTINATION .)

if(UNIX)
	set(CPACK_GENERATOR "DEB")
	set(CPACK_SOURCE_GENERATOR "TGZ")
	set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
	set(CPACK_DEBIAN_PACKAGE_SECTION "games")
	set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://klei1984.github.io/max/")
	set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")

	set(DEP_BIN_DIRS)
	foreach(TARGET_NAME ${${PROJECT_NAME}_deps})
			get_target_property(BIN_DIR ${TARGET_NAME} BINARY_DIR)
			list(APPEND DEP_BIN_DIRS "${BIN_DIR}")
			list(APPEND DEP_BIN_DIRS "${BIN_DIR}/lib")
	endforeach()

	set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS ${DEP_BIN_DIRS})

	install(TARGETS ${PROJECT_NAME}
		CONFIGURATIONS ${CMAKE_BUILD_TYPE}
		RUNTIME DESTINATION .
	)
endif()

if(MINGW)
	set(CPACK_GENERATOR "7Z")
	set(CPACK_SOURCE_GENERATOR "7Z")

	find_program(MAKENSIS_EXECUTABLE NAMES makensis DOC "The NSIS package maker command")
	if (MAKENSIS_EXECUTABLE)
		list(APPEND CPACK_GENERATOR "NSIS")
		set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
		set(CPACK_NSIS_MUI_FINISHPAGE_RUN "${PROJECT_NAME}")
		set(CPACK_NSIS_DISPLAY_NAME "M.A.X. Port")
		set(CPACK_NSIS_PACKAGE_NAME "M.A.X. Port v${GAME_VERSION}")
		set(CPACK_NSIS_URL_INFO_ABOUT "https://klei1984.github.io/max/")
	endif()

	install(TARGETS ${PROJECT_NAME}
		CONFIGURATIONS ${CMAKE_BUILD_TYPE}
		RUNTIME DESTINATION .
	)

	if (BUILD_SHARED_LIBS)
		set(DEP_BIN_DIRS)
		foreach(TARGET_NAME ${${PROJECT_NAME}_deps})
				get_target_property(BIN_DIR ${TARGET_NAME} BINARY_DIR)
				list(APPEND DEP_BIN_DIRS "${BIN_DIR}")
		endforeach()
	
		install(CODE "
			include(BundleUtilities)
			fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}\" \"\" \"${DEP_BIN_DIRS}\")
			"
		)
	endif()
endif()

INCLUDE(CPack)
