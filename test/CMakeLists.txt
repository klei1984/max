include(versions)
include(FetchContent)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${GTEST_FILE})
	file(${GTEST_HASH_TYPE} ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${GTEST_FILE} GTEST_FILE_HASH)

	if(${GTEST_FILE_HASH} STREQUAL ${GTEST_HASH})
		set(GTEST_URI file://${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${GTEST_FILE})
	endif()
endif()

FetchContent_Declare(
	GTest
	QUIET
	TIMEOUT 60
	URL ${GTEST_URI}
	URL_HASH ${GTEST_HASH_TYPE}=${GTEST_HASH}
	DOWNLOAD_EXTRACT_TIMESTAMP FALSE
	OVERRIDE_FIND_PACKAGE
)

set(GTEST_FORCE_SHARED_CRT ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(GTest)

add_executable(
    max_tests
    test_main.cpp
    smartpointer.cpp
    smartlist.cpp
    smartarray.cpp
    sortedarray.cpp
    registerarray.cpp
    ../src/registerarray.cpp
    smartfile.cpp
    ../src/smartfile.cpp
    smartobjectarray.cpp
)

target_compile_options(max_tests PRIVATE --coverage)
target_link_options(max_tests PRIVATE --coverage)

if(NOT BUILD_SHARED_LIBS)
	target_link_options(max_tests PRIVATE -static -static-libgcc -static-libstdc++)
	target_link_libraries(max_tests PRIVATE ${${PROJECT_NAME}_deps})
else()
	target_link_libraries(max_tests PRIVATE ${${PROJECT_NAME}_deps})
endif()

target_include_directories(max_tests PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${GAME_INCLUDES}
)

target_link_libraries(max_tests PRIVATE gtest)

include(GoogleTest)

gtest_discover_tests(max_tests PROPERTIES
	LABELS "unit tests"
	DISCOVERY_TIMEOUT 240
)
