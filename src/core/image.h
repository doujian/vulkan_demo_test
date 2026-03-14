#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace vk_core {

struct ImageCreateInfo {
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags usage = 0;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
};

struct ImageTransitionInfo {
    VkCommandBuffer cmd;
    VkImageLayout newLayout;
    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;
    VkAccessFlags srcAccess;
    VkAccessFlags dstAccess;
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
};

class Image {
public:
    Image() = default;
    ~Image();
    
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&&) noexcept;
    Image& operator=(Image&&) noexcept;
    
    bool init(const ImageCreateInfo& info);
    void destroy();
    
    VkImage get() const { return m_image; }
    VkImageView getView() const { return m_imageView; }
    VkFormat getFormat() const { return m_format; }
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    
    bool createImageView(VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1, uint32_t layerCount = 1);
    void transitionLayout(const ImageTransitionInfo& info);
    
    static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);
    static VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice,
                                        const std::vector<VkFormat>& candidates,
                                        VkImageTiling tiling, VkFormatFeatureFlags features);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_initialized = false;
};

class DepthImage : public Image {
public:
    bool init(VkDevice device, VkPhysicalDevice physicalDevice,
              uint32_t width, uint32_t height);
};

} // namespace vk_core