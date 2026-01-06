include(versions)
include(FetchContent)

if(EXISTS ${PROJECT_SOURCE_DIR}/dependencies/${MINIAUDIO_FILE})
	file(${MINIAUDIO_HASH_TYPE} ${PROJECT_SOURCE_DIR}/dependencies/${MINIAUDIO_FILE} MINIAUDIO_FILE_HASH)

	if(${MINIAUDIO_FILE_HASH} STREQUAL ${MINIAUDIO_HASH})
		set(MINIAUDIO_URI file://${PROJECT_SOURCE_DIR}/dependencies/${MINIAUDIO_FILE})
	endif()
endif()

find_package(Patch)

if(NOT Patch_FOUND)
	message(FATAL_ERROR "Patch tool is required.")
endif()

set(MINIAUDIO_PATCH ${Patch_EXECUTABLE} -p0 < ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/patches/miniaudio.patch)

FetchContent_Declare(
	MINIAUDIO
	TIMEOUT 60
	URL ${MINIAUDIO_URI}
	URL_HASH ${MINIAUDIO_HASH_TYPE}=${MINIAUDIO_HASH}
	DOWNLOAD_EXTRACT_TIMESTAMP FALSE
	OVERRIDE_FIND_PACKAGE
	PATCH_COMMAND ${MINIAUDIO_PATCH}
	UPDATE_DISCONNECTED TRUE
)

set(MINIAUDIO_NO_LIBVORBIS ON)
set(MINIAUDIO_NO_LIBOPUS ON)
set(MINIAUDIO_NO_ENCODING ON)
set(MINIAUDIO_NO_FLAC ON)
set(MINIAUDIO_NO_MP3 ON)
set(MINIAUDIO_NO_GENERATION ON)
set(MINIAUDIO_DISABLE_INSTALL ON)

# Force Release build without debug info (treat as system library)
set(CMAKE_BUILD_TYPE_BACKUP ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE Release)

FetchContent_MakeAvailable(MINIAUDIO)

# Restore build configuration
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_BACKUP})

if(NOT TARGET Miniaudio::Miniaudio)
	add_library(Miniaudio::Miniaudio ALIAS miniaudio)
endif()
