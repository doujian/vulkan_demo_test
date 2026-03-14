#pragma once

#include <vulkan/vulkan.h>

namespace vk_core {

class PipelineBase {
public:
    PipelineBase() = default;
    virtual ~PipelineBase();
    
    PipelineBase(const PipelineBase&) = delete;
    PipelineBase& operator=(const PipelineBase&) = delete;
    PipelineBase(PipelineBase&&) noexcept;
    PipelineBase& operator=(PipelineBase&&) noexcept;
    
    virtual void destroy();
    
    VkPipeline get() const { return m_pipeline; }
    VkPipelineLayout getLayout() const { return m_layout; }
    
    virtual void bind(VkCommandBuffer cmd) const = 0;

protected:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    bool m_initialized = false;
};

} // namespace vk_core