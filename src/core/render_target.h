#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include "image.h"

namespace vk_core {

class RenderTarget {
public:
    RenderTarget() = default;
    ~RenderTarget();
    
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    RenderTarget(RenderTarget&&) noexcept;
    RenderTarget& operator=(RenderTarget&&) noexcept;
    
    bool init(VkDevice device, VkPhysicalDevice physicalDevice,
              uint32_t width, uint32_t height, VkFormat colorFormat,
              VkImageUsageFlags additionalUsage = 0, bool enableDepth = false);
    void destroy();
    
    VkImage getColorImage() const { return m_colorImage.get(); }
    VkImageView getColorImageView() const { return m_colorImage.getView(); }
    VkImage getDepthImage() const { return m_depthImage.get(); }
    VkImageView getDepthImageView() const { return m_depthImage.getView(); }
    VkRenderPass getRenderPass() const { return m_renderPass; }
    VkFramebuffer getFramebuffer() const { return m_framebuffer; }
    VkExtent2D getExtent() const { return {m_colorImage.getWidth(), m_colorImage.getHeight()}; }
    VkFormat getFormat() const { return m_colorImage.getFormat(); }
    bool hasDepth() const { return m_hasDepth; }
    
    void transitionToColorAttachment(VkCommandBuffer cmd);
    void transitionToShaderRead(VkCommandBuffer cmd);
    void transitionToTransferSrc(VkCommandBuffer cmd);
    
    void beginRenderPass(VkCommandBuffer cmd, const float clearColor[4] = nullptr, float clearDepth = 1.0f);
    void endRenderPass(VkCommandBuffer cmd);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    
    Image m_colorImage;
    Image m_depthImage;
    bool m_hasDepth = false;
    
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    
    VkImageLayout m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    bool m_initialized = false;
};

} // namespace vk_core