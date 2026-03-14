#include "frame_buffer.h"

namespace vk_core {

FrameBuffer::FrameBuffer(FrameBuffer&& other) noexcept
    : m_device(other.m_device)
    , m_framebuffer(other.m_framebuffer)
    , m_extent(other.m_extent)
    , m_initialized(other.m_initialized) {
    other.m_framebuffer = VK_NULL_HANDLE;
    other.m_initialized = false;
}

FrameBuffer& FrameBuffer::operator=(FrameBuffer&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_framebuffer = other.m_framebuffer;
        m_extent = other.m_extent;
        m_initialized = other.m_initialized;
        other.m_framebuffer = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

FrameBuffer::~FrameBuffer() {
    destroy();
}

bool FrameBuffer::init(VkDevice device, VkRenderPass renderPass, VkExtent2D extent,
                       const std::vector<VkImageView>& attachments) {
    if (m_initialized) return true;
    
    m_device = device;
    m_extent = extent;
    
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;
    
    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_framebuffer) != VK_SUCCESS) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void FrameBuffer::destroy() {
    if (!m_initialized) return;
    
    if (m_framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
        m_framebuffer = VK_NULL_HANDLE;
    }
    
    m_initialized = false;
}

bool FrameBufferManager::init(VkDevice device, VkRenderPass renderPass,
                              const std::vector<VkImageView>& imageViews,
                              VkImageView depthView, VkExtent2D extent) {
    m_framebuffers.resize(imageViews.size());
    
    for (size_t i = 0; i < imageViews.size(); i++) {
        std::vector<VkImageView> attachments;
        attachments.push_back(imageViews[i]);
        if (depthView != VK_NULL_HANDLE) {
            attachments.push_back(depthView);
        }
        
        if (!m_framebuffers[i].init(device, renderPass, extent, attachments)) {
            return false;
        }
    }
    
    return true;
}

void FrameBufferManager::destroy() {
    for (auto& fb : m_framebuffers) {
        fb.destroy();
    }
    m_framebuffers.clear();
}

VkFramebuffer FrameBufferManager::get(size_t index) const {
    if (index < m_framebuffers.size()) {
        return m_framebuffers[index].get();
    }
    return VK_NULL_HANDLE;
}

} // namespace vk_core