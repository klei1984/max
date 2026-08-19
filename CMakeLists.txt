cmake_minimum_required (VERSION 3.28)

project(max LANGUAGES C CXX)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# Ensure BUILD_SHARED_LIBS defaults to OFF if not explicitly set
# Some dependencies (like Lua) set it to ON by default, which would override the preference
if(NOT DEFINED BUILD_SHARED_LIBS)
	set(BUILD_SHARED_LIBS OFF)
endif()

option(MAX_BUILD_TESTS "Build unit tests by default" ON)
option(MAX_ENABLE_UPNP "Use miniupnpc library" ON)

if(WIN32)
	set(MAX_SPLIT_DEBUG_SYMBOLS_DEFAULT ON)
else()
	set(MAX_SPLIT_DEBUG_SYMBOLS_DEFAULT OFF)
endif()

option(MAX_SPLIT_DEBUG_SYMBOLS
	"Split debug info into a sidecar, strip the executable and package the sidecar separately"
	${MAX_SPLIT_DEBUG_SYMBOLS_DEFAULT})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(setup)

# Ensure crash reporter works on 32 bit targets.
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	add_compile_options(-fasynchronous-unwind-tables)
endif()

set(GAME_INCLUDES "")
set(GAME_SOURCES "")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(${PROJECT_NAME} ${GAME_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "max-port")

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${GAME_INCLUDES}
)

if(GAME_VERSION_REVISION)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        GAME_VERSION_REVISION="${GAME_VERSION_REVISION}"
    )
endif()

if(GAME_VERSION_USE_BUILD_TIME)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
		GAME_VERSION_USE_BUILD_TIME=${GAME_VERSION_USE_BUILD_TIME}
    )
endif()


target_compile_definitions(${PROJECT_NAME} PRIVATE
    GAME_VERSION_MAJOR=${GAME_VERSION_MAJOR}
    GAME_VERSION_MINOR=${GAME_VERSION_MINOR}
    GAME_VERSION_PATCH=${GAME_VERSION_PATCH}
    SDL_ASSERT_LEVEL=2
)

if(MAX_ENABLE_UPNP)
	target_compile_definitions(${PROJECT_NAME} PUBLIC MAX_ENABLE_UPNP=1)
endif()

if(NOT BUILD_SHARED_LIBS)
	set(${PROJECT_NAME}_deps SDL3::SDL3-static utf8proc Miniaudio::Miniaudio Sha2::Sha2 nlohmann_json::nlohmann_json nlohmann_json_schema_validator::validator lua::static Xoshiro::Xoshiro Enet::Enet Freetype::Freetype Libbacktrace::Libbacktrace)

	if(MAX_ENABLE_UPNP)
		list(APPEND ${PROJECT_NAME}_deps miniupnpc::miniupnpc)
	endif()

	target_link_options(${PROJECT_NAME} PUBLIC -static -static-libgcc -static-libstdc++)
	target_link_libraries(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_deps})
else()
	set(${PROJECT_NAME}_deps SDL3::SDL3-shared utf8proc Miniaudio::Miniaudio Sha2::Sha2 nlohmann_json::nlohmann_json nlohmann_json_schema_validator::validator lua::shared Xoshiro::Xoshiro Enet::Enet Freetype::Freetype Libbacktrace::Libbacktrace)

	if(MAX_ENABLE_UPNP)
		list(APPEND ${PROJECT_NAME}_deps miniupnpc::miniupnpc)
	endif()

	target_link_libraries(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_deps})
endif()

if(MINGW)
	target_link_options(${PROJECT_NAME} PRIVATE -mwindows)
endif()

include(sdl3)
include(libbacktrace)
include(utf8proc)
include(freetype)
include(miniaudio)
include(sha2)
include(json)
include(jsonschema)
include(lua)
include(xoshiro)

set(ENET_RELEASE ON)
include(enet)

if(MAX_ENABLE_UPNP)
	include(miniupnp)
endif()

if(MAX_SPLIT_DEBUG_SYMBOLS)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_OBJCOPY} ARGS --only-keep-debug	 $<TARGET_FILE:${PROJECT_NAME}> $<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>.debug
		COMMAND ${CMAKE_STRIP} ARGS --strip-debug --strip-unneeded $<TARGET_FILE:${PROJECT_NAME}>
		COMMAND ${CMAKE_OBJCOPY} ARGS --add-gnu-debuglink $<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>.debug $<TARGET_FILE:${PROJECT_NAME}>
	)
endif()

if(MAX_BUILD_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()

set(CPACK_PACKAGE_VERSION_MAJOR ${GAME_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${GAME_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${GAME_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})

set(CPACK_PACKAGE_NAME "max-port")
set(CPACK_PACKAGE_VENDOR "max-port")
set(CPACK_PACKAGE_CONTACT "M.A.X. Port Team")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/assets/LICENSE")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "M.A.X. Port v${GAME_VERSION_STRING}")
set(CPACK_PACKAGE_DESCRIPTION "M.A.X. Port is an SDL library based runtime executable for the 1996 MS-DOS\n" "game M.A.X.: Mechanized Assault & Exploration developed and published by\n" "Interplay Productions.")
set(CPACK_SOURCE_IGNORE_FILES "/\.git/;/\.github/;/\.settings/;/\.vscode/;\.gitignore;.\.launch;\.project;\.cproject;\.clang-format;\.gdbinit;README.md;/doc/;/build/;${CMAKE_BINARY_DIR};Debug;Release;RelWithDebInfo;MinSizeRel;/dependencies/.*\.tar\.gz")

if(UNIX)
	set(CPACK_SOURCE_GENERATOR "TGZ")

	include(GNUInstallDirs)
	set(GAME_INSTALL_PATH "${CMAKE_INSTALL_DATADIR}/max-port")

	if(MAXPORT_FLATPAK_BUILD)
		set(GAME_INSTALL_LIB_PATH "${CMAKE_INSTALL_LIBDIR}")
	else()
		set(GAME_INSTALL_LIB_PATH "${CMAKE_INSTALL_LIBDIR}/max-port")
	endif()

	set(DEP_BIN_DIRS)
	set(DEP_LIBRARY_REGEXES)

	foreach(TARGET_NAME ${${PROJECT_NAME}_deps})
		get_target_property(BIN_DIR ${TARGET_NAME} BINARY_DIR)
		list(APPEND DEP_BIN_DIRS "${BIN_DIR}")
		list(APPEND DEP_BIN_DIRS "${BIN_DIR}/lib")

		get_target_property(ALIASED_TARGET ${TARGET_NAME} ALIASED_TARGET)

		if(ALIASED_TARGET)
			set(DEP_TARGET ${ALIASED_TARGET})
		else()
			set(DEP_TARGET ${TARGET_NAME})
		endif()

		get_target_property(DEP_TARGET_TYPE ${DEP_TARGET} TYPE)

		if(DEP_TARGET_TYPE STREQUAL "SHARED_LIBRARY")
			list(APPEND DEP_LIBRARY_REGEXES
				"^$<TARGET_FILE_PREFIX:${DEP_TARGET}>$<TARGET_FILE_BASE_NAME:${DEP_TARGET}>.*$")
		endif()
	endforeach()

	if(DEP_LIBRARY_REGEXES)
		install(CODE "set(CMAKE_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\")")
		install(CODE "set(GAME_INSTALL_LIB_PATH \"${GAME_INSTALL_LIB_PATH}\")")
		install(CODE "set(DEP_BIN_DIRS \"${DEP_BIN_DIRS}\")")
		install(CODE "set(DEP_LIBRARY_REGEXES \"${DEP_LIBRARY_REGEXES}\")")
		install(CODE [[
		file(GET_RUNTIME_DEPENDENCIES
			EXECUTABLES $<TARGET_FILE:max>
			RESOLVED_DEPENDENCIES_VAR DEPS_RESOLVED
			UNRESOLVED_DEPENDENCIES_VAR DEPS_UNRESOLVED
			CONFLICTING_DEPENDENCIES_PREFIX DEPS_CONFLICTING
			DIRECTORIES ${DEP_BIN_DIRS}
			PRE_EXCLUDE_REGEXES
				 ".*"
			PRE_INCLUDE_REGEXES
				${DEP_LIBRARY_REGEXES}
		)

		if(DEPS_CONFLICTING_FILENAMES)
			message(WARNING "Conflicting dependencies for library: \"${DEPS_CONFLICTING}\"!")
		endif()

		foreach(DEP_REGEX ${DEP_LIBRARY_REGEXES})
			set(DEP_REGEX_MATCHED FALSE)

			foreach(DEP_FILE ${DEPS_RESOLVED})
				get_filename_component(DEP_FILE_NAME "${DEP_FILE}" NAME)

				if(DEP_FILE_NAME MATCHES "${DEP_REGEX}")
					set(DEP_REGEX_MATCHED TRUE)
					break()
				endif()
			endforeach()

			if(NOT DEP_REGEX_MATCHED)
				message(FATAL_ERROR "No shared dependency matches \"${DEP_REGEX}\", the package would "
					"be missing a library that the game needs at runtime.")
			endif()
		endforeach()

		foreach(DEP_FILE ${DEPS_RESOLVED})
			file(INSTALL
				FILES "${DEP_FILE}"
				FOLLOW_SYMLINK_CHAIN
				DESTINATION "${CMAKE_INSTALL_PREFIX}/${GAME_INSTALL_LIB_PATH}"
			)
		endforeach()
		]])
	endif()

	if(MAXPORT_FLATPAK_BUILD)
		set(MAXPORT_LAUNCHER_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
	elseif(MAXPORT_PKGMAKE_BUILD)
		set(MAXPORT_LAUNCHER_DIR "/usr/bin")
	else()
		set(MAXPORT_LAUNCHER_DIR "/usr/games")
	endif()

	set_target_properties(${PROJECT_NAME} PROPERTIES
		INSTALL_RPATH "$ORIGIN/../${GAME_INSTALL_LIB_PATH};$ORIGIN")

	install(TARGETS ${PROJECT_NAME}
		CONFIGURATIONS ${CMAKE_BUILD_TYPE}
		RUNTIME DESTINATION "${MAXPORT_LAUNCHER_DIR}"
	)

	configure_file("${CMAKE_SOURCE_DIR}/assets/max-port.desktop.in" "${CMAKE_BINARY_DIR}/max-port.desktop" @ONLY)

	if(MAXPORT_FLATPAK_BUILD)
		configure_file("${CMAKE_SOURCE_DIR}/flatpak/metainfo.xml.in" "${CMAKE_BINARY_DIR}/metainfo.xml" @ONLY)
	else()
		install(FILES "${CMAKE_BINARY_DIR}/max-port.desktop"
			DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
		)

		install(FILES "${CMAKE_SOURCE_DIR}/assets/max.png"
			DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps"
			RENAME "max-port.png"
		)
	endif()
endif()

if(MINGW)
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "M.A.X. Port")
	set(GAME_INSTALL_PATH ".")
	set(CPACK_GENERATOR "7Z")
	set(CPACK_SOURCE_GENERATOR "7Z")
	set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_SOURCE_DIR}/cmake/scripts/cpack_exclude_portable.cmake")

	include(nsis/nsis)

	install(TARGETS ${PROJECT_NAME}
		CONFIGURATIONS ${CMAKE_BUILD_TYPE}
		RUNTIME DESTINATION .
	)

	if(BUILD_SHARED_LIBS)
		set(DEP_BIN_DIRS)
		foreach(TARGET_NAME ${${PROJECT_NAME}_deps})
				get_target_property(BIN_DIR ${TARGET_NAME} BINARY_DIR)
				list(APPEND DEP_BIN_DIRS "${BIN_DIR}")
		endforeach()

		install(CODE "
			include(BundleUtilities)
			fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/$<TARGET_FILE_NAME:${PROJECT_NAME}>\" \"\" \"${DEP_BIN_DIRS}\")
			"
		)
	endif()
endif()

add_subdirectory(assets)

if(MAX_SPLIT_DEBUG_SYMBOLS)
	set(CPACK_MAX_SYMBOLS_FILE "${CMAKE_BINARY_DIR}/max-port.debug")
	set(CPACK_MAX_SYMBOLS_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

	if(MINGW)
		set(CPACK_MAX_SYMBOLS_GENERATOR "7Z")
		set(CPACK_MAX_SYMBOLS_FORMAT "7zip")
		set(CPACK_MAX_SYMBOLS_TAR_FLAGS "cf")
		set(CPACK_MAX_SYMBOLS_SUFFIX "-symbols.7z")
	else()
		set(CPACK_MAX_SYMBOLS_GENERATOR "TGZ")
		set(CPACK_MAX_SYMBOLS_FORMAT "gnutar")
		set(CPACK_MAX_SYMBOLS_TAR_FLAGS "czf")
		set(CPACK_MAX_SYMBOLS_SUFFIX "-symbols.tar.gz")
	endif()

	list(APPEND CPACK_POST_BUILD_SCRIPTS "${CMAKE_SOURCE_DIR}/cmake/scripts/cpack_symbols_package.cmake")
endif()

include(CPack)
