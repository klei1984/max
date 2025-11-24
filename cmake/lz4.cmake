include(versions)
include(FetchContent)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${LZ4_FILE})
	file(${LZ4_HASH_TYPE} ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${LZ4_FILE} LZ4_FILE_HASH)

	if(${LZ4_FILE_HASH} STREQUAL ${LZ4_HASH})
		set(LZ4_URI file://${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${LZ4_FILE})
	endif()
endif()

set(LZ4_BUILD_CLI OFF CACHE BOOL "Do not build lz4 CLI program")
set(LZ4_POSITION_INDEPENDENT_LIB ON CACHE BOOL "Use position independent code")
set(BUILD_STATIC_LIBS ON CACHE BOOL "Always build static library")

FetchContent_Declare(
	LZ4
	TIMEOUT 60
	URL ${LZ4_URI}
	URL_HASH ${LZ4_HASH_TYPE}=${LZ4_HASH}
	DOWNLOAD_EXTRACT_TIMESTAMP FALSE
	SOURCE_SUBDIR build/cmake
	OVERRIDE_FIND_PACKAGE
	UPDATE_DISCONNECTED TRUE
)

FetchContent_MakeAvailable(LZ4)

if(TARGET lz4_static AND NOT TARGET lz4::static)
	add_library(lz4::static ALIAS lz4_static)
endif()

if(TARGET lz4_shared AND NOT TARGET lz4::shared)
	add_library(lz4::shared ALIAS lz4_shared)
endif()
