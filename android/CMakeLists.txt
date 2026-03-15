cmake_minimum_required(VERSION 3.22)
project(vulkan_demo_android VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_VALIDATION "Enable Vulkan validation layers" OFF)
option(BUILD_ANDROID_CLI "Build Android CLI version" ON)
option(BUILD_ANDROID_APP "Build Android APK version" ON)

if(NOT ANDROID)
    message(FATAL_ERROR "This CMakeLists is for Android only. Use the root CMakeLists.txt for Windows/Linux.")
endif()

find_package(Vulkan REQUIRED)

set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/ktx)
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
    ../src/core/instance.cpp
    ../src/core/device.cpp
    ../src/core/swapchain.cpp
    ../src/core/command.cpp
    ../src/core/buffer.cpp
    ../src/core/image.cpp
    ../src/core/render_pass.cpp
    ../src/core/pipeline.cpp
    ../src/core/pipeline_base.cpp
    ../src/core/frame_buffer.cpp
    ../src/core/descriptor.cpp
    ../src/core/shader.cpp
    ../src/core/sync.cpp
    ../src/core/sampler.cpp
    ../src/core/compute_pipeline.cpp
    ../src/core/render_target.cpp
)

set(UTILS_SOURCES
    ../src/utils/png_writer.cpp
    ../src/utils/args_parser.cpp
    ../src/utils/file_utils.cpp
)

set(TEST_SOURCES
    ../src/test/image_comparator.cpp
    ../src/test/test_runner.cpp
)

set(DEMO_SOURCES
    ../src/demo/demo_registry.cpp
)

set(OFFSCREEN_SOURCES
    ../src/offscreen/offscreen_renderer.cpp
)

set(MODEL_SOURCES
    ../src/model/gltf_model.cpp
    ../src/model/texture_loader.cpp
)

set(ANDROID_COMMON_SOURCES
    android_platform.cpp
)

add_library(libktx STATIC ${KTX_SOURCES})
target_include_directories(libktx PUBLIC
    ${KTX_DIR}/include
    ${KTX_DIR}/lib
    ${KTX_DIR}/other_include
    ${KTX_DIR}/utils
)
target_link_libraries(libktx PUBLIC Vulkan::Vulkan)

add_library(vulkan_core STATIC
    ${CORE_SOURCES}
    ${UTILS_SOURCES}
    ${TEST_SOURCES}
    ${DEMO_SOURCES}
    ${OFFSCREEN_SOURCES}
    ${MODEL_SOURCES}
    ${ANDROID_COMMON_SOURCES}
)

target_include_directories(vulkan_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tinygltf
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/stb
    ${KTX_DIR}/include
    ${KTX_DIR}/other_include
    ${KTX_DIR}/lib
)

target_link_libraries(vulkan_core PUBLIC Vulkan::Vulkan libktx android log)

if(ENABLE_VALIDATION)
    target_compile_definitions(vulkan_core PUBLIC ENABLE_VALIDATION_LAYERS)
endif()

target_compile_definitions(vulkan_core PUBLIC ENABLE_OFFSCREEN VK_USE_PLATFORM_ANDROID_KHR)

add_subdirectory(../src/demo/demos demos)

if(BUILD_ANDROID_CLI)
    add_executable(vulkan_demo_cli
        ../src/main.cpp
        ../src/app.cpp
    )
    target_link_libraries(vulkan_demo_cli PRIVATE vulkan_core vulkan_demos log)
    set_target_properties(vulkan_demo_cli PROPERTIES
        OUTPUT_NAME vulkan_demo
    )
endif()

if(BUILD_ANDROID_APP)
    add_library(vulkan_demo_app SHARED
        android_app.cpp
    )
    target_link_libraries(vulkan_demo_app PRIVATE vulkan_core vulkan_demos log android)
    set_target_properties(vulkan_demo_app PROPERTIES
        OUTPUT_NAME vulkan_demo
    )
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../golden DESTINATION ${CMAKE_BINARY_DIR})