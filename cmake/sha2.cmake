include(versions)
include(FetchContent)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${SHA2_FILE})
	file(${SHA2_HASH_TYPE} ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${SHA2_FILE} SHA2_FILE_HASH)

	if(${SHA2_FILE_HASH} STREQUAL ${SHA2_HASH})
		set(SHA2_URI file://${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${SHA2_FILE})
	endif()
endif()

FetchContent_Declare(
	SHA2
	TIMEOUT 60
	URL ${SHA2_URI}
	URL_HASH ${SHA2_HASH_TYPE}=${SHA2_HASH}
	DOWNLOAD_EXTRACT_TIMESTAMP FALSE
	OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(SHA2)

set(SHA2_BINARY_DIR ${PROJECT_BINARY_DIR}/_deps/sha2-build)
set(SHA2_SOURCE_DIR ${PROJECT_BINARY_DIR}/_deps/sha2-src)

file(GLOB SHA2_FILES CONFIGURE_DEPENDS
	"${SHA2_SOURCE_DIR}/*.c"
	"${SHA2_SOURCE_DIR}/*.h"
)

file(COPY ${SHA2_FILES} DESTINATION ${SHA2_BINARY_DIR})

add_library(Sha2 STATIC "${SHA2_BINARY_DIR}/sha2.c")

target_include_directories(Sha2 PUBLIC "${SHA2_BINARY_DIR}")

if(NOT TARGET Sha2::Sha2)
	add_library(Sha2::Sha2 ALIAS Sha2)
endif()
