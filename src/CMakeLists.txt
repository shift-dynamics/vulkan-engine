file(GLOB_RECURSE
    includes RELATIVE
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
foreach(include ${includes})
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/${include}"
        "${CMAKE_BINARY_DIR}/include/vulkan-engine/${include}"
        COPYONLY
    )
endforeach()

find_package(Qt5 COMPONENTS Core Widgets Gui REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(shaders)

set(vulkan_engine_src
    ${CMAKE_CURRENT_SOURCE_DIR}/Workspace.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Workspace.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/Camera.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/OrbitalCamera.cc
    # ${CMAKE_CURRENT_SOURCE_DIR}/Mesh.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Shader.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TriangleRenderer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/VulkanWindow.cc
)
add_library(vulkan_engine STATIC ${vulkan_engine_src})

target_link_libraries(vulkan_engine Qt5::Core Qt5::Widgets Qt5::Gui)

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
  target_compile_definitions(
    vulkan_engine
    PRIVATE
    QT_NO_DEBUG
  )
endif()

target_compile_options(
  vulkan_engine
  PRIVATE
  -march=x86-64
  -mtune=generic
  -fstack-protector-strong
  --param=ssp-buffer-size=4
  -W
  -lstdc++
  -Wall
  -Wextra
  -g3
  -O3
  -std=c++11
)

target_include_directories(vulkan_engine PUBLIC ${CMAKE_BINARY_DIR}/include)

add_executable(viewer
    main.cc
)
target_link_libraries(viewer PRIVATE
    vulkan_engine
    shaders
)
