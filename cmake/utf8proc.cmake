include(versions)
include(FetchContent)

if(EXISTS ${PROJECT_SOURCE_DIR}/dependencies/${UTF8PROC_FILE})
	file(${UTF8PROC_HASH_TYPE} ${PROJECT_SOURCE_DIR}/dependencies/${UTF8PROC_FILE} UTF8PROC_FILE_HASH)

	if(${UTF8PROC_FILE_HASH} STREQUAL ${UTF8PROC_HASH})
		set(UTF8PROC_URI file://${PROJECT_SOURCE_DIR}/dependencies/${UTF8PROC_FILE})
	endif()
endif()

FetchContent_Declare(
	UTF8PROC
	TIMEOUT 60
	URL ${UTF8PROC_URI}
	URL_HASH ${UTF8PROC_HASH_TYPE}=${UTF8PROC_HASH}
	DOWNLOAD_EXTRACT_TIMESTAMP FALSE
	OVERRIDE_FIND_PACKAGE
)

set(UTF8PROC_INSTALL Off)

# Force Release build without debug info (treat as system library)
set(CMAKE_BUILD_TYPE_BACKUP ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE Release)

FetchContent_MakeAvailable(UTF8PROC)

# Restore build configuration
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_BACKUP})
