find_package(Git)

if(Git_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --always --tags --match "v[0-9]*"
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		RESULT_VARIABLE GIT_RESULT
		OUTPUT_VARIABLE GIT_OUTPUT
		ERROR_QUIET
	)

	if(NOT GIT_RESULT EQUAL 0)
		set(GIT_OUTPUT "")
	endif()

	string(STRIP "${GIT_OUTPUT}" GIT_OUTPUT)

	string(REGEX MATCH "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)\\-([0-9]+)\\-([a-zA-Z0-9]+)$" GIT_MATCHES "${GIT_OUTPUT}")
	# format: <release tag>-<commits since tag>-<commit object short id> example: v0.7.0-29-gd036209
	if(CMAKE_MATCH_COUNT EQUAL 5)
		set(GAME_VERSION_MAJOR ${CMAKE_MATCH_1})
		set(GAME_VERSION_MINOR ${CMAKE_MATCH_2})
		set(GAME_VERSION_PATCH ${CMAKE_MATCH_3})
		set(GAME_VERSION_BUILD ${CMAKE_MATCH_4})
		set(GAME_VERSION_REVISION ${CMAKE_MATCH_5})
	else()
		# format: <release tag> example: v0.7.0
		string(REGEX MATCH "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)$" GIT_MATCHES "${GIT_OUTPUT}")
		if(CMAKE_MATCH_COUNT EQUAL 3)
			set(GAME_VERSION_MAJOR ${CMAKE_MATCH_1})
			set(GAME_VERSION_MINOR ${CMAKE_MATCH_2})
			set(GAME_VERSION_PATCH ${CMAKE_MATCH_3})
		else()
			# format: no release tag is reachable - development build
			set(GAME_VERSION_MAJOR "0")
			set(GAME_VERSION_MINOR "8")
			set(GAME_VERSION_PATCH "0")

			# format: <commit object short id> example: b5b42f7
			string(REGEX MATCH "^[0-9a-fA-F]+$" GIT_MATCHES "${GIT_OUTPUT}")
			if(GIT_MATCHES)
				set(GAME_VERSION_REVISION ${GIT_OUTPUT})
			else()
				# format: unknown - ask git for the commit object short id directly
				execute_process(
					COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
					WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
					RESULT_VARIABLE GIT_RESULT
					OUTPUT_VARIABLE GIT_REVISION
					ERROR_QUIET
					OUTPUT_STRIP_TRAILING_WHITESPACE
				)

				if(GIT_RESULT EQUAL 0 AND GIT_REVISION)
					set(GAME_VERSION_REVISION ${GIT_REVISION})
				else()
					# format: no source control - custom build
					set(GAME_VERSION_USE_BUILD_TIME TRUE)
				endif()
			endif()
		endif()
	endif()

	# The version info is evaluated at configure time.
	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse --git-dir
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_DIR
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse --git-common-dir
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_COMMON_DIR
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	# a checked out branch keeps the commit object id in its ref file, a detached HEAD does not
	execute_process(
		COMMAND ${GIT_EXECUTABLE} symbolic-ref --quiet HEAD
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_HEAD_REF
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	set(GIT_WATCHED_FILES "")

	if(GIT_DIR)
		get_filename_component(GIT_DIR "${GIT_DIR}" ABSOLUTE BASE_DIR "${PROJECT_SOURCE_DIR}")

		if(EXISTS "${GIT_DIR}/HEAD")
			list(APPEND GIT_WATCHED_FILES "${GIT_DIR}/HEAD")
		endif()
	endif()

	if(GIT_COMMON_DIR)
		get_filename_component(GIT_COMMON_DIR "${GIT_COMMON_DIR}" ABSOLUTE BASE_DIR "${PROJECT_SOURCE_DIR}")

		if(GIT_HEAD_REF AND EXISTS "${GIT_COMMON_DIR}/${GIT_HEAD_REF}")
			list(APPEND GIT_WATCHED_FILES "${GIT_COMMON_DIR}/${GIT_HEAD_REF}")
		elseif(EXISTS "${GIT_COMMON_DIR}/packed-refs")
			list(APPEND GIT_WATCHED_FILES "${GIT_COMMON_DIR}/packed-refs")
		endif()
	endif()

	if(GIT_WATCHED_FILES)
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${GIT_WATCHED_FILES})
	endif()
else()
	# format: no source control - custom build
	set(GAME_VERSION_MAJOR "0")
	set(GAME_VERSION_MINOR "8")
	set(GAME_VERSION_PATCH "0")
	set(GAME_VERSION_USE_BUILD_TIME TRUE)
endif()

if(GAME_VERSION_REVISION)
	set(GAME_VERSION_INFO "Revision ${GAME_VERSION_REVISION}")
elseif(GAME_VERSION_USE_BUILD_TIME)
	set(GAME_VERSION_INFO "Build time")
else()
	set(GAME_VERSION_INFO "Release")
endif()

message(STATUS
  "Build version info:\n"
  "  Version  : v${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_PATCH}\n"
  "  Revision : ${GAME_VERSION_INFO}\n"
)
