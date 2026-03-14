cmake_minimum_required(VERSION 3.16)

set(DEMO_SOURCES
    triangle/demo_triangle.cpp
    multi_pass_blend/demo_multi_pass_blend.cpp
)

add_library(vulkan_demos STATIC ${DEMO_SOURCES})
target_include_directories(vulkan_demos PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(vulkan_demos PUBLIC vulkan_core)