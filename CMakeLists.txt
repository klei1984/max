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
option(MAX_ENABLE_INSTALLER "Enable installer module for asset conversion" ON)
option(MAX_BUILD_INSTALLER_STANDALONE "Build standalone installer shared library (independent of BUILD_SHARED_LIBS)" OFF)

# Enable RmlUi debugger by default in debug builds, disable in release builds
# (debugger contains SIL-licensed fonts that should not be in release builds)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	option(ENABLE_RMLUI_DEBUGGER "Enable RmlUi debugger (contains SIL-licensed fonts)" ON)
else()
	option(ENABLE_RMLUI_DEBUGGER "Enable RmlUi debugger (contains SIL-licensed fonts)" OFF)
endif()

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

if(GAME_VERSION_REVISION)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        GAME_VERSION_REVISION="${GAME_VERSION_REVISION}"
    )
endif()

if(GAME_VERSION_REVISION)
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

if(MAX_ENABLE_INSTALLER)
	target_compile_definitions(${PROJECT_NAME} PUBLIC MAX_ENABLE_INSTALLER=1)
endif()

if(ENABLE_RMLUI_DEBUGGER)
	target_compile_definitions(${PROJECT_NAME} PUBLIC ENABLE_RMLUI_DEBUGGER=1)
endif()

if(NOT BUILD_SHARED_LIBS)
	set(${PROJECT_NAME}_deps SDL3::SDL3-static utf8proc Miniaudio::Miniaudio Sha2::Sha2 nlohmann_json::nlohmann_json nlohmann_json_schema_validator::validator lua::static lz4::static rectpack::rectpack Xoshiro::Xoshiro Enet::Enet RmlUi::Core Freetype::Freetype)

	if(ENABLE_RMLUI_DEBUGGER)
		list(APPEND ${PROJECT_NAME}_deps RmlUi::Debugger)
	endif()

	if(MAX_ENABLE_UPNP)
		list(APPEND ${PROJECT_NAME}_deps miniupnpc::miniupnpc)
	endif()

	if(MAX_ENABLE_INSTALLER)
		list(APPEND ${PROJECT_NAME}_deps max_installer_static)
	endif()

	target_link_options(${PROJECT_NAME} PUBLIC -static -static-libgcc -static-libstdc++)
	target_link_libraries(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_deps})
else()
	set(${PROJECT_NAME}_deps SDL3::SDL3-shared utf8proc Miniaudio::Miniaudio Sha2::Sha2 nlohmann_json::nlohmann_json nlohmann_json_schema_validator::validator lua::shared lz4::shared rectpack::rectpack Xoshiro::Xoshiro Enet::Enet RmlUi::Core Freetype::Freetype)

	if(ENABLE_RMLUI_DEBUGGER)
		list(APPEND ${PROJECT_NAME}_deps RmlUi::Debugger)
	endif()

	if(MAX_ENABLE_UPNP)
		list(APPEND ${PROJECT_NAME}_deps miniupnpc::miniupnpc)
	endif()

	if(MAX_ENABLE_INSTALLER)
		# When building game as shared library, also use shared installer if available
		# Otherwise fall back to static installer library
		if(MAX_BUILD_INSTALLER_STANDALONE)
			list(APPEND ${PROJECT_NAME}_deps max_installer_shared)
		else()
			list(APPEND ${PROJECT_NAME}_deps max_installer_static)
		endif()
	endif()

	target_link_libraries(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_deps})
endif()

if(MINGW)
	target_link_options(${PROJECT_NAME} PRIVATE -mwindows)
endif()

include(sdl3)
include(utf8proc)
include(freetype)
include(rmlui)
include(miniaudio)
include(sha2)
include(json)
include(jsonschema)
include(lua)
include(lz4)
include(rectpack2d)
include(xoshiro)

set(ENET_RELEASE ON)
include(enet)

if(MAX_ENABLE_UPNP)
	include(miniupnp)
endif()

# Conditionally add installer subdirectory if feature is enabled
if(MAX_ENABLE_INSTALLER)
	add_subdirectory(src/installer)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_OBJCOPY} ARGS --only-keep-debug	 $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.debug
	COMMAND ${CMAKE_STRIP} ARGS --strip-debug --strip-unneeded $<TARGET_FILE:${PROJECT_NAME}>
	COMMAND ${CMAKE_OBJCOPY} ARGS --add-gnu-debuglink ${PROJECT_NAME}.debug $<TARGET_FILE:${PROJECT_NAME}>
)

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
set(CPACK_SOURCE_IGNORE_FILES "/\.git/;/\.github/;/\.settings/;/\.vscode/;\.gitignore;.\.launch;\.project;\.cproject;\.clang-format;\.gdbinit;README.md;/doc/;${CMAKE_BINARY_DIR};Debug;Release;RelWithDebInfo;MinSizeRel;/dependencies/.*\.tar\.gz")

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
	foreach(TARGET_NAME ${${PROJECT_NAME}_deps})
		get_target_property(BIN_DIR ${TARGET_NAME} BINARY_DIR)
		list(APPEND DEP_BIN_DIRS "${BIN_DIR}")
		list(APPEND DEP_BIN_DIRS "${BIN_DIR}/lib")
	endforeach()

	install(CODE "set(CMAKE_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\")")
	install(CODE "set(GAME_INSTALL_LIB_PATH \"${GAME_INSTALL_LIB_PATH}\")")
	install(CODE "set(DEP_BIN_DIRS \"${DEP_BIN_DIRS}\")")
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
				"^libSDL3.*$"
				"^libfreetype.*$"
				"^libminiupnpc.*$"
				"^libnlohmann_json.*$"
				"^libnlohmann_json_schema_validator.*$"
				"^liblua.*$"
				"^liblz4.*$"
				"^libutf8proc.*$"
				"^libRml.*$"
		)

		if(DEPS_CONFLICTING_FILENAMES)
			message(WARNING "Conflicting dependencies for library: \"${DEPS_CONFLICTING}\"!")
		endif()

		foreach(DEP_FILE ${DEPS_RESOLVED})
			file(INSTALL
				FILES "${DEP_FILE}"
				FOLLOW_SYMLINK_CHAIN
				DESTINATION "${CMAKE_INSTALL_PREFIX}/${GAME_INSTALL_LIB_PATH}"
			)
		endforeach()
	]])

	install(TARGETS ${PROJECT_NAME}
		CONFIGURATIONS ${CMAKE_BUILD_TYPE}
		RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/${GAME_INSTALL_LIB_PATH}"
	)

	configure_file("${CMAKE_SOURCE_DIR}/assets/max-port.in" "${CMAKE_BINARY_DIR}/max-port" @ONLY)
	configure_file("${CMAKE_SOURCE_DIR}/assets/max-port.desktop.in" "${CMAKE_BINARY_DIR}/max-port.desktop" @ONLY)

	if(MAXPORT_FLATPAK_BUILD)
		configure_file("${CMAKE_SOURCE_DIR}/flatpak/metainfo.xml.in" "${CMAKE_BINARY_DIR}/metainfo.xml" @ONLY)
	else()
		if(MAXPORT_PKGMAKE_BUILD)
			install(FILES "${CMAKE_BINARY_DIR}/max-port"
				DESTINATION "/usr/bin"
				PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
			)
		else()
			install(FILES "${CMAKE_BINARY_DIR}/max-port"
				DESTINATION "/usr/games"
				PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
			)
		endif()

		install(FILES "${CMAKE_BINARY_DIR}/max-port.desktop"
			DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
		)

		install(FILES "${CMAKE_SOURCE_DIR}/assets/max.png"
			DESTINATION "${GAME_INSTALL_PATH}"
		)
	endif()
endif()

if(MINGW)
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "M.A.X. Port")
	set(GAME_INSTALL_PATH ".")
	set(CPACK_GENERATOR "7Z")
	set(CPACK_SOURCE_GENERATOR "7Z")

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
			fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}\" \"\" \"${DEP_BIN_DIRS}\")
			"
		)
	endif()
endif()

add_subdirectory(assets)

include(CPack)
