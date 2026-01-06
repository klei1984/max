include(versions)
include(FetchContent)

if(EXISTS ${PROJECT_SOURCE_DIR}/dependencies/${SDL3_FILE})
	file(${SDL3_HASH_TYPE} ${PROJECT_SOURCE_DIR}/dependencies/${SDL3_FILE} SDL3_FILE_HASH)

	if(${SDL3_FILE_HASH} STREQUAL ${SDL3_HASH})
		set(SDL3_URI file://${PROJECT_SOURCE_DIR}/dependencies/${SDL3_FILE})
	endif()
endif()

FetchContent_Declare(
	SDL3
	TIMEOUT 60
	URL ${SDL3_URI}
	URL_HASH ${SDL3_HASH_TYPE}=${SDL3_HASH}
	DOWNLOAD_EXTRACT_TIMESTAMP FALSE
	OVERRIDE_FIND_PACKAGE
)

set(SDL_HAPTIC OFF)
set(SDL_HIDAPI OFF)
set(SDL_JOYSTICK OFF)
set(SDL_OPENGLES OFF)
set(SDL_SENSOR OFF)
set(SDL_TEST_LIBRARY OFF)
set(SDL_DISABLE_INSTALL ON)

# Force Release build without debug info (treat as system library)
set(CMAKE_BUILD_TYPE_BACKUP ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE Release)

FetchContent_MakeAvailable(SDL3)

# Restore build configuration
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_BACKUP})
