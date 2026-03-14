#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace vk_core {

class RenderPass {
public:
    RenderPass() = default;
    ~RenderPass();
    
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&&) noexcept;
    RenderPass& operator=(RenderPass&&) noexcept;
    
    bool init(VkDevice device, VkFormat colorFormat, VkFormat depthFormat = VK_FORMAT_UNDEFINED,
              bool clearOnLoad = true, bool presentSrc = true);
    bool initOffscreen(VkDevice device, VkFormat colorFormat, VkFormat depthFormat = VK_FORMAT_UNDEFINED,
                       bool clearOnLoad = true);
    void destroy();
    
    VkRenderPass get() const { return m_renderPass; }
    
    void begin(VkCommandBuffer cmd, VkFramebuffer framebuffer, VkExtent2D extent,
               const std::vector<VkClearValue>& clearValues) const;
    void end(VkCommandBuffer cmd) const;

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    bool m_initialized = false;
};

} // namespace vk_core