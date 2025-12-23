include(versions)
include(FetchContent)

if(EXISTS ${PROJECT_SOURCE_DIR}/dependencies/${RMLUI_FILE})
	file(${RMLUI_HASH_TYPE} ${PROJECT_SOURCE_DIR}/dependencies/${RMLUI_FILE} RMLUI_FILE_HASH)

	if(${RMLUI_FILE_HASH} STREQUAL ${RMLUI_HASH})
		set(RMLUI_URI file://${PROJECT_SOURCE_DIR}/dependencies/${RMLUI_FILE})
	endif()
endif()

FetchContent_Declare(
	RmlUi
	TIMEOUT 120
	URL ${RMLUI_URI}
	URL_HASH ${RMLUI_HASH_TYPE}=${RMLUI_HASH}
	DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)

set(RMLUI_SAMPLES OFF CACHE BOOL "Build samples")
set(RMLUI_BACKEND "SDL_GL3" CACHE STRING "Backend to use")
set(RMLUI_FONT_ENGINE "freetype" CACHE STRING "Font engine to use")
set(RMLUI_THIRDPARTY_CONTAINERS ON CACHE BOOL "Use integrated third-party containers")
set(RMLUI_CUSTOM_RTTI OFF CACHE BOOL "Use custom RTTI implementation")
set(RMLUI_PRECOMPILED_HEADERS OFF CACHE BOOL "Disable precompiled headers")
set(RMLUI_TRACY_PROFILING OFF CACHE BOOL "Disable Tracy profiling")
set(BUILD_TESTING OFF CACHE BOOL "Disable RmlUi tests")
set(RMLUI_INSTALL OFF CACHE BOOL "Disable RmlUI installation")

FetchContent_MakeAvailable(RmlUi)
