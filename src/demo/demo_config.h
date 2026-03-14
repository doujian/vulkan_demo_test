#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace vk_demo {

struct DemoConfig {
    uint32_t width = 800;
    uint32_t height = 600;
    bool enableDepth = true;
    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_SRGB;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    std::string vertexShader;
    std::string fragmentShader;
    std::vector<const char*> requiredExtensions;
    VkClearColorValue clearColor = {{0.0f, 0.0f, 0.0f, 1.0f}};
};

inline DemoConfig getDefaultConfig() {
    DemoConfig config;
    config.depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;
    config.vertexShader = "shaders/default.vert.spv";
    config.fragmentShader = "shaders/default.frag.spv";
    return config;
}

} // namespace vk_demo