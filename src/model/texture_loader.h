#pragma once

#include <vulkan/vulkan.h>
#include <string>

namespace vk_model {

struct Texture {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorImageInfo descriptor{};
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
    uint32_t layerCount = 1;

    void destroy(VkDevice device);
};

class TextureLoader {
public:
    static bool loadCubemap(const std::string& filename, VkDevice device, VkPhysicalDevice physicalDevice, 
                            VkQueue queue, Texture& texture);
    static bool loadTexture2D(const std::string& filename, VkDevice device, VkPhysicalDevice physicalDevice,
                              VkQueue queue, Texture& texture);
};

}