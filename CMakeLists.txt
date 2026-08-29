cmake_minimum_required(VERSION 3.16)
set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "Minimum policy version" FORCE)

if(APPLE AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures")
endif()

project(TowerDefenseCHAI3D VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Global compile definitions for macOS / CHAI3D
if(APPLE)
    add_compile_definitions(MACOSX __MACOSX_CORE__ GL_SILENCE_DEPRECATION)
    link_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/external/chai3d/externals/DHD/lib/mac-arm64
    )
endif()

# Build CHAI3D subproject
add_subdirectory(external/chai3d)

# Include directories for CHAI3D and project headers
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external/chai3d/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external/chai3d/externals/Eigen
    ${CMAKE_CURRENT_SOURCE_DIR}/external/chai3d/externals/glew/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/chai3d/extras/GLFW/include
)

# Executable sources
set(GAME_SOURCES
    src/main.cpp
    src/Turret.cpp
    src/Bullet.cpp
    src/Monster.cpp
    src/Game.cpp
)

add_executable(TowerDefenseCHAI3D ${GAME_SOURCES})

target_link_libraries(TowerDefenseCHAI3D
    chai3d
    glfw
    drd
)

if(APPLE)
    target_link_libraries(TowerDefenseCHAI3D
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework CoreMIDI"
    )
endif()

# Ensure assets directory is created
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets")

# Ensure assets directory is copied next to binary
add_custom_command(TARGET TowerDefenseCHAI3D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "${CMAKE_BINARY_DIR}/assets"
    COMMENT "Copying assets folder to build directory"
)
