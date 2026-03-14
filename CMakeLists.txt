cmake_minimum_required(VERSION 3.16)
project(vulkan_demo_test VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_VALIDATION "Enable Vulkan validation layers" ON)
option(ENABLE_OFFSCREEN "Enable offscreen rendering" ON)
option(BUILD_WINDOWS_GUI "Build Windows GUI" OFF)

set(VULKAN_SDK "E:/VulkanSDK/1.4.341.1")
set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK}/Include")
set(Vulkan_LIBRARY "${VULKAN_SDK}/Lib/vulkan-1.lib")

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw_src)

find_package(Vulkan REQUIRED)

set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ktx)
set(KTX_SOURCES
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/filestream.c
    ${KTX_DIR}/lib/vkloader.c
)

set(CORE_SOURCES
    src/core/instance.cpp
    src/core/device.cpp
    src/core/swapchain.cpp
    src/core/command.cpp
    src/core/buffer.cpp
    src/core/image.cpp
    src/core/render_pass.cpp
    src/core/pipeline.cpp
    src/core/pipeline_base.cpp
    src/core/frame_buffer.cpp
    src/core/descriptor.cpp
    src/core/shader.cpp
    src/core/sync.cpp
    src/core/sampler.cpp
    src/core/compute_pipeline.cpp
    src/core/render_target.cpp
)

set(UTILS_SOURCES
    src/utils/png_writer.cpp
    src/utils/args_parser.cpp
    src/utils/file_utils.cpp
)

set(TEST_SOURCES
    src/test/image_comparator.cpp
    src/test/test_runner.cpp
)

set(DEMO_SOURCES
    src/demo/demo_registry.cpp
)

set(OFFSCREEN_SOURCES
    src/offscreen/offscreen_renderer.cpp
)

set(MODEL_SOURCES
    src/model/gltf_model.cpp
    src/model/texture_loader.cpp
)

add_library(libktx STATIC ${KTX_SOURCES})
target_include_directories(libktx PUBLIC
    ${KTX_DIR}/include
    ${KTX_DIR}/lib
    ${KTX_DIR}/other_include
    ${KTX_DIR}/utils
    ${Vulkan_INCLUDE_DIRS}
)
target_link_libraries(libktx PUBLIC Vulkan::Vulkan)
set_property(TARGET libktx PROPERTY FOLDER "external")

add_library(vulkan_core STATIC
    ${CORE_SOURCES}
    ${UTILS_SOURCES}
    ${TEST_SOURCES}
    ${DEMO_SOURCES}
    ${OFFSCREEN_SOURCES}
    ${MODEL_SOURCES}
)

target_include_directories(vulkan_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinygltf
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
    ${KTX_DIR}/include
    ${KTX_DIR}/other_include
    ${KTX_DIR}/lib
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(vulkan_core PUBLIC Vulkan::Vulkan glfw libktx)

if(ENABLE_VALIDATION)
    target_compile_definitions(vulkan_core PUBLIC ENABLE_VALIDATION_LAYERS)
endif()

if(ENABLE_OFFSCREEN)
    target_compile_definitions(vulkan_core PUBLIC ENABLE_OFFSCREEN)
endif()

add_executable(vulkan_demo
    src/main.cpp
    src/app.cpp
)

target_link_libraries(vulkan_demo PRIVATE vulkan_core vulkan_demos glfw)

if(WIN32)
    target_link_libraries(vulkan_demo PRIVATE user32 gdi32)
endif()

set_target_properties(vulkan_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET vulkan_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:vulkan_demo>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/golden
        $<TARGET_FILE_DIR:vulkan_demo>/golden
)

add_subdirectory(src/demo/demos)

install(TARGETS vulkan_demo RUNTIME DESTINATION bin)