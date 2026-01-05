cmake_minimum_required(VERSION 3.28)

project(max_installer LANGUAGES CXX)

set(INSTALLER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/installer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/procedures.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/progress_event.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sound.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/voices.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soundeffects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/music.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/flic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/flics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/portraits.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mve_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/videos.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shadow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sprite.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sprites.cpp
)

set(INSTALLER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/installer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/procedures.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/progress.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/progress_event.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sound.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/voices.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soundeffects.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/music.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/flic.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/flics.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/control.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panel.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/portraits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mve_parser.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/videos.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/palette.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shadow.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sprite.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sprites.hpp
)

add_library(max_installer_static STATIC ${INSTALLER_SOURCES} ${INSTALLER_HEADERS})

set_target_properties(max_installer_static PROPERTIES OUTPUT_NAME max_installer)

target_include_directories(max_installer_static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Note: Static library is embedded in game executable which already links miniaudio
target_include_directories(max_installer_static PRIVATE
    $<TARGET_PROPERTY:Miniaudio::Miniaudio,INTERFACE_INCLUDE_DIRECTORIES>
)

# Link SDL3 - prefer static but fall back to shared if static is not available (e.g., on Linux)
if(TARGET SDL3::SDL3-static)
    target_link_libraries(max_installer_static PUBLIC SDL3::SDL3-static)
else()
    target_link_libraries(max_installer_static PUBLIC SDL3::SDL3-shared)
endif()

# Link LZ4 - prefer static but fall back to shared if static is not available
if(TARGET lz4::static)
    target_link_libraries(max_installer_static PUBLIC lz4::static)
else()
    target_link_libraries(max_installer_static PUBLIC lz4::shared)
endif()

target_link_libraries(max_installer_static PUBLIC
    nlohmann_json::nlohmann_json
    rectpack::rectpack
    Sha2::Sha2
)

target_compile_definitions(max_installer_static PUBLIC MAX_INSTALLER_STATIC=1)

if(MAX_BUILD_INSTALLER_STANDALONE)
    message(STATUS "Building standalone installer shared library")
    
    add_library(max_installer_shared SHARED ${INSTALLER_SOURCES} ${INSTALLER_HEADERS})
    set_target_properties(max_installer_shared PROPERTIES OUTPUT_NAME max_installer)
    target_include_directories(max_installer_shared PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
    
    # Shared library must be self-contained with all dependencies for standalone installation use
    if(TARGET SDL3::SDL3-shared)
        target_link_libraries(max_installer_shared PUBLIC SDL3::SDL3-shared)
    else()
        target_link_libraries(max_installer_shared PUBLIC SDL3::SDL3-static)
    endif()
    
    if(TARGET lz4::shared)
        target_link_libraries(max_installer_shared PUBLIC lz4::shared)
    else()
        target_link_libraries(max_installer_shared PUBLIC lz4::static)
    endif()
    
    target_link_libraries(max_installer_shared PUBLIC
        nlohmann_json::nlohmann_json
        Miniaudio::Miniaudio
        rectpack::rectpack
        Sha2::Sha2
    )
    
    target_compile_definitions(max_installer_shared PUBLIC MAX_INSTALLER_SHARED=1)
endif()

set(INSTALLER_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
