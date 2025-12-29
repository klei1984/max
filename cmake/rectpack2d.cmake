include(versions)
include(FetchContent)

if(EXISTS ${PROJECT_SOURCE_DIR}/dependencies/${RECTPACK2D_FILE})
	file(${RECTPACK2D_HASH_TYPE} ${PROJECT_SOURCE_DIR}/dependencies/${RECTPACK2D_FILE} RECTPACK2D_FILE_HASH)

	if(${RECTPACK2D_FILE_HASH} STREQUAL ${RECTPACK2D_HASH})
		set(RECTPACK2D_URI file://${PROJECT_SOURCE_DIR}/dependencies/${RECTPACK2D_FILE})
	endif()
endif()

FetchContent_Declare(
	RECTPACK2D
	TIMEOUT 60
	URL ${RECTPACK2D_URI}
	URL_HASH ${RECTPACK2D_HASH_TYPE}=${RECTPACK2D_HASH}
	DOWNLOAD_EXTRACT_TIMESTAMP FALSE
	OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(RECTPACK2D)

set(RECTPACK2D_BINARY_DIR ${PROJECT_BINARY_DIR}/_deps/rectpack2d-build)
set(RECTPACK2D_SOURCE_DIR ${PROJECT_BINARY_DIR}/_deps/rectpack2d-src)

file(COPY ${RECTPACK2D_SOURCE_DIR}/src/rectpack2D DESTINATION ${RECTPACK2D_BINARY_DIR}/include)

add_library(rectpack INTERFACE)
set_property(TARGET rectpack APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${RECTPACK2D_BINARY_DIR}/include")

add_dependencies(rectpack RECTPACK2D)

if(NOT TARGET rectpack::rectpack)
	add_library(rectpack::rectpack ALIAS rectpack)
endif()
