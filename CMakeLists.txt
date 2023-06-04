cmake_minimum_required (VERSION 3.10)

project(max)
set(GAME_BINARY "max")

find_package(Git)

if(Git_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --always --tags
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
			set(GAME_VERSION_MINOR "4")
			set(GAME_VERSION_PATCH "2")
			set(GAME_VERSION "${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_PATCH} Revision ${GAME_VERSION_REVISION}")
		endif()
	endif()
else()
	set(GAME_VERSION_MAJOR "0")
	set(GAME_VERSION_MINOR "4")
	set(GAME_VERSION_PATCH "2")
	set(GAME_VERSION "${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_PATCH}")
endif()

message(STATUS "Set build version to v${GAME_VERSION}")

enable_language(C CXX)

set(GAME_INCLUDES "")
set(GAME_SOURCES "")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/")

if (MINGW)
	find_package(Freetype)
	find_package(Iconv)
	find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
	find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
	find_package(SDL2_mixer REQUIRED CONFIG REQUIRED COMPONENTS SDL2_mixer)
	find_package(SDL2_net REQUIRED CONFIG REQUIRED COMPONENTS SDL2_net)
endif (MINGW)

if (UNIX)
	set(ENV{PKG_CONFIG_PATH} "/usr/lib/i386-linux-gnu/pkgconfig")

	INCLUDE(FindPkgConfig)

	PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
	#PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2_main)
	PKG_SEARCH_MODULE(SDL2_mixer REQUIRED SDL2_mixer)
	PKG_SEARCH_MODULE(SDL2_net REQUIRED SDL2_net>=2.0.0)
	PKG_SEARCH_MODULE(Freetype REQUIRED freetype2)
	#PKG_SEARCH_MODULE(Iconv REQUIRED iconv)

	if(NOT TARGET SDL2::SDL2)
		add_library(SDL2::SDL2 INTERFACE IMPORTED)
		set_property(TARGET SDL2::SDL2 APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${SDL2_LIBRARIES})
		set_property(TARGET SDL2::SDL2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIRS})
		add_library(SDL2::SDL2main INTERFACE IMPORTED)
		set_property(TARGET SDL2::SDL2main APPEND PROPERTY INTERFACE_LINK_LIBRARIES )
		set_property(TARGET SDL2::SDL2main PROPERTY INTERFACE_INCLUDE_DIRECTORIES )
	endif()

	if(NOT TARGET SDL2_net::SDL2_net)
		add_library(SDL2_net::SDL2_net INTERFACE IMPORTED)
		set_property(TARGET SDL2_net::SDL2_net APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${SDL2_net_LIBRARIES})
		set_property(TARGET SDL2_net::SDL2_net PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SDL2_net_INCLUDE_DIRS})
	endif()

	if(NOT TARGET SDL2_mixer::SDL2_mixer)
		add_library(SDL2_mixer::SDL2_mixer INTERFACE IMPORTED)
		set_property(TARGET SDL2_mixer::SDL2_mixer APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${SDL2_mixer_LIBRARIES})
		set_property(TARGET SDL2_mixer::SDL2_mixer PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SDL2_mixer_INCLUDE_DIRS})
	endif()

	if(NOT TARGET Iconv::Iconv)
		add_library(Iconv::Iconv INTERFACE IMPORTED)
		set_property(TARGET Iconv::Iconv APPEND PROPERTY INTERFACE_LINK_LIBRARIES )
		set_property(TARGET Iconv::Iconv PROPERTY INTERFACE_INCLUDE_DIRECTORIES )
	endif()

	if(NOT TARGET Freetype::Freetype)
		add_library(Freetype::Freetype INTERFACE IMPORTED)
		set_property(TARGET Freetype::Freetype APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${Freetype_LIBRARIES})
		set_property(TARGET Freetype::Freetype PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Freetype_INCLUDE_DIRS})
	endif()
endif (UNIX)

add_executable(${GAME_BINARY} ${GAME_SOURCES})

target_include_directories(${GAME_BINARY} PRIVATE
	SDL2::SDL2main Iconv::Iconv Freetype::Freetype SDL2::SDL2 SDL2_net::SDL2_net SDL2_mixer::SDL2_mixer
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${GAME_INCLUDES}
)

set_target_properties(${GAME_BINARY} PROPERTIES
	CXX_STANDARD "20"
	COMPILE_FLAGS "-m32 -Wcomments -Wignored-qualifiers -Wshadow=local -Wtype-limits -Wparentheses -Wuninitialized -Wmaybe-uninitialized -Wformat -Wformat-overflow -Wformat-truncation -Wlogical-op -Wlogical-not-parentheses -fno-eliminate-unused-debug-types -DGAME_VERSION=\"\\\"v${GAME_VERSION}\\\"\""
	LINK_FLAGS "-m32")

target_link_libraries(${GAME_BINARY} PRIVATE SDL2::SDL2main Iconv::Iconv Freetype::Freetype SDL2::SDL2 SDL2_net::SDL2_net SDL2_mixer::SDL2_mixer)

add_custom_command(TARGET ${GAME_BINARY} POST_BUILD
	COMMAND ${CMAKE_OBJCOPY} ARGS --only-keep-debug	 $<TARGET_FILE:${GAME_BINARY}> ${GAME_BINARY}.debug
	COMMAND ${CMAKE_STRIP} ARGS --strip-debug --strip-unneeded $<TARGET_FILE:${GAME_BINARY}>
	COMMAND ${CMAKE_OBJCOPY} ARGS --add-gnu-debuglink ${GAME_BINARY}.debug $<TARGET_FILE:${GAME_BINARY}>
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
set(CPACK_SOURCE_IGNORE_FILES "/\.git/;/\.github/;/\.settings/;\.gitignore;.\.launch;\.project;\.cproject;\.clang-format;\.gdbinit;README.md;/doc/;${CMAKE_BINARY_DIR}")

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

	install(TARGETS ${GAME_BINARY}
		CONFIGURATIONS ${CMAKE_BUILD_TYPE}
		RUNTIME DESTINATION .
	)
endif()

if(MINGW)
	set(CPACK_GENERATOR "NSIS;7Z")
	set(CPACK_SOURCE_GENERATOR "7Z")
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
