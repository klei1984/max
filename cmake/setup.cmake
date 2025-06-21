find_package(Git)

if(Git_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --always --tags
		RESULT_VARIABLE GIT_RESULT
		OUTPUT_VARIABLE GIT_OUTPUT
		ERROR_QUIET
	)

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
			# format: unknown - custom build
			set(GAME_VERSION_MAJOR "0")
			set(GAME_VERSION_MINOR "7")
			set(GAME_VERSION_PATCH "2")
			set(GAME_VERSION_USE_BUILD_TIME TRUE)
		endif()
	endif()
else()
	# format: no source control - custom build
	set(GAME_VERSION_MAJOR "0")
	set(GAME_VERSION_MINOR "7")
	set(GAME_VERSION_PATCH "2")
	set(GAME_VERSION_USE_BUILD_TIME TRUE)
endif()

message(STATUS
  "Build version info:\n"
  "  Version  : v${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_PATCH}\n"
  "  Revision : ${GAME_VERSION_REVISION}-${GAME_VERSION_BUILD}\n"
)
