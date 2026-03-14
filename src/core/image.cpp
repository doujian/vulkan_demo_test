#include "image.h"
#include "device.h"
#include <stdexcept>

namespace vk_core {

Image::Image(Image&& other) noexcept
    : m_device(other.m_device)
    , m_image(other.m_image)
    , m_memory(other.m_memory)
    , m_imageView(other.m_imageView)
    , m_format(other.m_format)
    , m_layout(other.m_layout)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_initialized(other.m_initialized) {
    other.m_image = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_imageView = VK_NULL_HANDLE;
    other.m_initialized = false;
}

Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_image = other.m_image;
        m_memory = other.m_memory;
        m_imageView = other.m_imageView;
        m_format = other.m_format;
        m_layout = other.m_layout;
        m_width = other.m_width;
        m_height = other.m_height;
        m_initialized = other.m_initialized;
        other.m_image = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

Image::~Image() {
    destroy();
}

bool Image::init(const ImageCreateInfo& info) {
    if (m_initialized) return true;
    
    m_device = info.device;
    m_format = info.format;
    m_width = info.width;
    m_height = info.height;
    
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = info.width;
    imageInfo.extent.height = info.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = info.mipLevels;
    imageInfo.arrayLayers = info.arrayLayers;
    imageInfo.format = info.format;
    imageInfo.tiling = info.tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = info.usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (!Device::createImageWithMemory(info.device, info.physicalDevice, &imageInfo, info.properties, &m_image, &m_memory)) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void Image::destroy() {
    if (!m_initialized) return;
    
    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }
    
    if (m_image != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
    }
    
    if (m_memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_memory, nullptr);
        m_memory = VK_NULL_HANDLE;
    }
    
    m_initialized = false;
}

bool Image::createImageView(VkImageAspectFlags aspectFlags, uint32_t mipLevels, uint32_t layerCount) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = layerCount;
    
    return vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView) == VK_SUCCESS;
}

void Image::transitionLayout(const ImageTransitionInfo& info) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_layout;
    barrier.newLayout = info.newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = info.aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = info.srcAccess;
    barrier.dstAccessMask = info.dstAccess;
    
    vkCmdPipelineBarrier(info.cmd, info.srcStage, info.dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    m_layout = info.newLayout;
}

VkFormat Image::findDepthFormat(VkPhysicalDevice physicalDevice) {
    return findSupportedFormat(physicalDevice,
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat Image::findSupportedFormat(VkPhysicalDevice physicalDevice,
                                    const std::vector<VkFormat>& candidates,
                                    VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    return VK_FORMAT_UNDEFINED;
}

bool DepthImage::init(VkDevice device, VkPhysicalDevice physicalDevice,
                      uint32_t width, uint32_t height) {
    VkFormat depthFormat = findDepthFormat(physicalDevice);
    
    ImageCreateInfo info{};
    info.device = device;
    info.physicalDevice = physicalDevice;
    info.width = width;
    info.height = height;
    info.format = depthFormat;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    if (!Image::init(info)) {
        return false;
    }
    
    return createImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
}

} // namespace vk_core