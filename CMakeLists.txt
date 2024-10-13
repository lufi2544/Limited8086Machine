cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(Limited86Machine VERSION 1.0 LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable PDB generation in Debug mode
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")  # Generates PDB files
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")  # Include PDB during linking

# Use file(GLOB ...) to include all .cpp files in the source and dtest directories
file(GLOB_RECURSE SOURCES
    "source/*.cpp")

# Add the executable with all gathered source files
add_executable(Limited86Machine ${SOURCES})# Add the executable

# Optionally add include directories
target_include_directories(Limited86Machine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_SOURCE_DIR}/dtest
)