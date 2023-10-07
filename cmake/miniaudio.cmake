include(versions)
include(FetchContent)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${MINIAUDIO_FILE})
	file(${MINIAUDIO_HASH_TYPE} ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${MINIAUDIO_FILE} MINIAUDIO_FILE_HASH)

	if(${MINIAUDIO_FILE_HASH} STREQUAL ${MINIAUDIO_HASH})
		set(MINIAUDIO_URI file://${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${MINIAUDIO_FILE})
	endif()
endif()

FetchContent_Declare(
	MINIAUDIO
	TIMEOUT 60
	URL ${MINIAUDIO_URI}
	URL_HASH ${MINIAUDIO_HASH_TYPE}=${MINIAUDIO_HASH}
	DOWNLOAD_EXTRACT_TIMESTAMP FALSE
	OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(MINIAUDIO)

set(MINIAUDIO_BINARY_DIR ${PROJECT_BINARY_DIR}/_deps/miniaudio-build)
set(MINIAUDIO_SOURCE_DIR ${PROJECT_BINARY_DIR}/_deps/miniaudio-src)

file(COPY ${MINIAUDIO_SOURCE_DIR}/miniaudio.h DESTINATION ${MINIAUDIO_BINARY_DIR}/include)

add_library(Miniaudio INTERFACE)
set_property(TARGET Miniaudio APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${MINIAUDIO_BINARY_DIR}/include")

if(NOT TARGET Miniaudio::Miniaudio)
	add_library(Miniaudio::Miniaudio ALIAS Miniaudio)
endif()
