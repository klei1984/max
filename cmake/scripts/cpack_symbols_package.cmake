# Copyright: 2026 M.A.X. Port Team
# License: MIT
#
# CPack post-build hook that archives the debug info sidecar produced by the
# POST_BUILD split in the top level CMakeLists.txt.
#
# The archive is deliberately kept out of the main package: it is one to two
# orders of magnitude larger than the game and only a developer resolving a
# crash report ever needs it. Users who never open a backtrace never download
# it.
#
# CPack sources this once per generator pass, source package runs included, so
# every condition it depends on is checked here rather than assumed.

if(NOT DEFINED CPACK_MAX_SYMBOLS_FILE OR NOT EXISTS "${CPACK_MAX_SYMBOLS_FILE}")
	return()
endif()

# One archive per cpack invocation, not one per generator. On Windows the
# generator list is "7Z;NSIS" and both passes would otherwise redo several
# hundred megabytes of compression for an identical result.
if(NOT CPACK_GENERATOR STREQUAL CPACK_MAX_SYMBOLS_GENERATOR)
	return()
endif()

# Source packages carry no build output, so a run configured from
# CPackSourceConfig.cmake has nothing to do here.
if(DEFINED CPACK_SOURCE_PACKAGE_FILE_NAME AND
   CPACK_PACKAGE_FILE_NAME STREQUAL CPACK_SOURCE_PACKAGE_FILE_NAME)
	return()
endif()

set(SYMBOLS_ARCHIVE
	"${CPACK_MAX_SYMBOLS_OUTPUT_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}${CPACK_MAX_SYMBOLS_SUFFIX}")

get_filename_component(SYMBOLS_DIRECTORY "${CPACK_MAX_SYMBOLS_FILE}" DIRECTORY)
get_filename_component(SYMBOLS_NAME "${CPACK_MAX_SYMBOLS_FILE}" NAME)

message(STATUS "Packaging debug symbols: ${SYMBOLS_ARCHIVE}")

# Run from the sidecar's own directory so the archive holds a bare file name
# rather than a copy of the build tree layout. CrashReporter looks for the
# sidecar next to the executable, so anything else would make the user dig it
# out of nested directories.
execute_process(
	COMMAND ${CMAKE_COMMAND} -E tar "${CPACK_MAX_SYMBOLS_TAR_FLAGS}" "${SYMBOLS_ARCHIVE}"
		"--format=${CPACK_MAX_SYMBOLS_FORMAT}" "${SYMBOLS_NAME}"
	WORKING_DIRECTORY "${SYMBOLS_DIRECTORY}"
	RESULT_VARIABLE SYMBOLS_RESULT
	OUTPUT_VARIABLE SYMBOLS_OUTPUT
	ERROR_VARIABLE SYMBOLS_OUTPUT
)

if(NOT SYMBOLS_RESULT EQUAL 0)
	message(WARNING "Failed to package debug symbols: ${SYMBOLS_OUTPUT}")
endif()
