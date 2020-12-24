cmake_minimum_required(VERSION 3.11)
project(vulkan-engine
    VERSION 0.0.1
    DESCRIPTION "Lightweight rendering engine based on Qt/Vulkan"
    LANGUAGES C CXX
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
message(STATUS "Building ${PROJECT_NAME} in ${CMAKE_BUILD_TYPE} mode.")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -O0" CACHE STRING "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
include_directories(${CMAKE_BINARY_DIR}/include)

find_package(assimp)
# include(FetchContent)
# FetchContent_Declare(
#     assimp
#     GIT_REPOSITORY https://github.com/assimp/assimp.git
#     GIT_TAG v5.0.1
# )
# FetchContent_MakeAvailable(assimp)

add_subdirectory(src)

