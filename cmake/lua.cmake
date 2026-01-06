include(versions)
include(FetchContent)

if(EXISTS ${PROJECT_SOURCE_DIR}/dependencies/${LUA_FILE})
	file(${LUA_HASH_TYPE} ${PROJECT_SOURCE_DIR}/dependencies/${LUA_FILE} LUA_FILE_HASH)

	if(${LUA_FILE_HASH} STREQUAL ${LUA_HASH})
		set(LUA_URI file://${PROJECT_SOURCE_DIR}/dependencies/${LUA_FILE})
	endif()
endif()

set(LUA_PATCH ${Patch_EXECUTABLE} -p0 < ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/patches/lua.patch)

FetchContent_Declare(
	LUA
	TIMEOUT 60
	URL ${LUA_URI}
	URL_HASH ${LUA_HASH_TYPE}=${LUA_HASH}
	DOWNLOAD_EXTRACT_TIMESTAMP FALSE
	OVERRIDE_FIND_PACKAGE
	PATCH_COMMAND ${LUA_PATCH}
	UPDATE_DISCONNECTED TRUE
)

# Force Release build without debug info (treat as system library)
set(CMAKE_BUILD_TYPE_BACKUP ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE Release)

FetchContent_MakeAvailable(LUA)

# Restore build configuration
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_BACKUP})
