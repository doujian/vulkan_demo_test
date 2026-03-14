#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include "../core/image.h"

namespace vk_demo {
class DemoBase;
}

namespace vk_core {
    class Instance;
    class Device;
    class Command;
    class Image;
    class RenderPass;
    class FrameBuffer;
}

namespace vk_offscreen {

class OffscreenRenderer {
public:
    OffscreenRenderer() = default;
    ~OffscreenRenderer();
    
    bool init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device,
              uint32_t width, uint32_t height, VkFormat colorFormat,
              bool enableDepth = true, VkFormat depthFormat = VK_FORMAT_UNDEFINED);
    void destroy();
    
    VkImage getColorImage() const;
    VkImageView getColorImageView() const;
    VkRenderPass getRenderPass() const;
    VkFramebuffer getFramebuffer() const;
    VkExtent2D getExtent() const;
    
    void beginRenderPass(VkCommandBuffer cmd) const;
    void endRenderPass(VkCommandBuffer cmd) const;
    
    bool saveToPNG(const std::string& filename, VkQueue queue, VkCommandPool cmdPool) const;

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    
    vk_core::Image m_colorImage;
    vk_core::DepthImage m_depthImage;
    
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    
    bool m_initialized = false;
};

} // namespace vk_offscreen