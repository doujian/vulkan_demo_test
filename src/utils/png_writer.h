#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>

namespace vk_utils {

struct SaveImageInfo {
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    uint32_t width = 0;
    uint32_t height = 0;
    std::string filename;
};

bool saveImageToPNG(const std::string& filename, const uint8_t* data, 
                    uint32_t width, uint32_t height, uint32_t channels = 4);

bool saveVkImageToPNG(const SaveImageInfo& info);

} // namespace vk_utils