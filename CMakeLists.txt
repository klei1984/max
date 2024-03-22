cmake_minimum_required (VERSION 3.24)

project(max LANGUAGES C CXX)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

option(MAX_BUILD_TESTS "Build unit tests by default" ON)
option(MAX_ENABLE_UPNP "Use miniupnpc library" ON)

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
	COMPILE_FLAGS "${MAX_CFLAGS} -DGAME_VERSION_STRING=\"\\\"v${GAME_VERSION_STRING}\\\"\" -DGAME_VERSION_MAJOR=\"${GAME_VERSION_MAJOR}\" -DGAME_VERSION_MINOR=\"${GAME_VERSION_MINOR}\" -DGAME_VERSION_PATCH=\"${GAME_VERSION_PATCH}\" -DSDL_ASSERT_LEVEL=2"
)

if(MAX_ENABLE_UPNP)
	target_compile_definitions(${PROJECT_NAME} PUBLIC MAX_ENABLE_UPNP=1)
endif()

if(NOT BUILD_SHARED_LIBS)
	set(${PROJECT_NAME}_deps SDL2::SDL2main Iconv::Iconv Freetype::Freetype Miniaudio::Miniaudio Enet::Enet SDL2::SDL2-static)

	if(MAX_ENABLE_UPNP)
		list(APPEND ${PROJECT_NAME}_deps miniupnpc::miniupnpc)
	endif()

	target_link_options(${PROJECT_NAME} PUBLIC -static -static-libgcc -static-libstdc++)
	target_link_libraries(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_deps})
else()
	set(${PROJECT_NAME}_deps SDL2::SDL2main Iconv::Iconv Freetype::Freetype Miniaudio::Miniaudio Enet::Enet SDL2::SDL2)

	if(MAX_ENABLE_UPNP)
		list(APPEND ${PROJECT_NAME}_deps miniupnpc::miniupnpc)
	endif()

	target_link_libraries(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_deps})
endif()

if(MINGW)
	target_link_options(${PROJECT_NAME} PRIVATE -mwindows)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cmake/lib-iconv ${CMAKE_CURRENT_BINARY_DIR}/_deps/iconv-build)

include(sdl2)
include(freetype)
include(miniaudio)

set(ENET_RELEASE ON)
include(enet)

if(MAX_ENABLE_UPNP)
	include(miniupnp)
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
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "M.A.X. Port v${GAME_VERSION_STRING}")
set(CPACK_PACKAGE_DESCRIPTION "M.A.X. Port is an SDL library based runtime executable for the 1996 MS-DOS game M.A.X.: Mechanized Assault & Exploration developed and published by Interplay Productions.")
set(CPACK_SOURCE_IGNORE_FILES "/\.git/;/\.github/;/\.settings/;\.gitignore;.\.launch;\.project;\.cproject;\.clang-format;\.gdbinit;README.md;/doc/;${CMAKE_BINARY_DIR};Debug;Release;RelWithDebInfo;MinSizeRel;/dependencies/.*\.tar\.gz")

if(UNIX)
	include(GNUInstallDirs)
	set(CPACK_GENERATOR "DEB")
	set(CPACK_SOURCE_GENERATOR "TGZ")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
	set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
	set(CPACK_DEBIAN_PACKAGE_SECTION "games")
	set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://klei1984.github.io/max/")

	if(X86)
		set (CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
	elseif(X86_64)
		set (CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
	endif()

	set(DEP_BIN_DIRS)
	foreach(TARGET_NAME ${${PROJECT_NAME}_deps})
			get_target_property(BIN_DIR ${TARGET_NAME} BINARY_DIR)
			list(APPEND DEP_BIN_DIRS "${BIN_DIR}")
			list(APPEND DEP_BIN_DIRS "${BIN_DIR}/lib")
	endforeach()

	set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS ${DEP_BIN_DIRS})

	install(CODE "set(PROJECT_NAME \"${PROJECT_NAME}\")")
	install(CODE "set(CMAKE_INSTALL_BINDIR \"${CMAKE_INSTALL_BINDIR}\")")
	install(CODE "set(CMAKE_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\")")
	install(CODE "set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS \"${DEP_BIN_DIRS}\")")
	install(CODE [[
		file(GET_RUNTIME_DEPENDENCIES
			EXECUTABLES max
			RESOLVED_DEPENDENCIES_VAR DEPS_RESOLVED
			UNRESOLVED_DEPENDENCIES_VAR DEPS_UNRESOLVED
			CONFLICTING_DEPENDENCIES_PREFIX DEPS_CONFLICTING
			DIRECTORIES ${CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS}
		)

		if(DEPS_CONFLICTING_FILENAMES)
			message(WARNING "Conflicting dependencies for library: \"${DEPS_CONFLICTING}\"!")
		endif()

		foreach(DEP_FILE ${DEPS_RESOLVED})
			file(INSTALL
				FILES "${DEP_FILE}"
				FOLLOW_SYMLINK_CHAIN
				DESTINATION ${CMAKE_INSTALL_PREFIX}
			)
		endforeach()
	]])

	install(TARGETS ${PROJECT_NAME}
		CONFIGURATIONS ${CMAKE_BUILD_TYPE}
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
	)
endif()

if(MINGW)
	set(CMAKE_INSTALL_BINDIR "." CACHE PATH "User executables directory" FORCE)
	set(CMAKE_INSTALL_DATADIR "." CACHE PATH "Read-only architecture-independent data directory" FORCE)
	include(GNUInstallDirs)
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
