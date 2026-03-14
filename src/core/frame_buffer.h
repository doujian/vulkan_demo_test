#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace vk_core {

class FrameBuffer {
public:
    FrameBuffer() = default;
    ~FrameBuffer();
    
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;
    FrameBuffer(FrameBuffer&&) noexcept;
    FrameBuffer& operator=(FrameBuffer&&) noexcept;
    
    bool init(VkDevice device, VkRenderPass renderPass, VkExtent2D extent,
              const std::vector<VkImageView>& attachments);
    void destroy();
    
    VkFramebuffer get() const { return m_framebuffer; }
    VkExtent2D getExtent() const { return m_extent; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    VkExtent2D m_extent = {0, 0};
    bool m_initialized = false;
};

class FrameBufferManager {
public:
    bool init(VkDevice device, VkRenderPass renderPass,
              const std::vector<VkImageView>& imageViews,
              VkImageView depthView, VkExtent2D extent);
    void destroy();
    
    VkFramebuffer get(size_t index) const;
    size_t getCount() const { return m_framebuffers.size(); }

private:
    std::vector<FrameBuffer> m_framebuffers;
};

} // namespace vk_core