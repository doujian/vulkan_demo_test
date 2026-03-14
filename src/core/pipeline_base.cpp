#include "pipeline_base.h"

namespace vk_core {

PipelineBase::PipelineBase(PipelineBase&& other) noexcept
    : m_device(other.m_device)
    , m_pipeline(other.m_pipeline)
    , m_layout(other.m_layout)
    , m_initialized(other.m_initialized) {
    other.m_pipeline = VK_NULL_HANDLE;
    other.m_layout = VK_NULL_HANDLE;
    other.m_initialized = false;
}

PipelineBase& PipelineBase::operator=(PipelineBase&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_pipeline = other.m_pipeline;
        m_layout = other.m_layout;
        m_initialized = other.m_initialized;
        other.m_pipeline = VK_NULL_HANDLE;
        other.m_layout = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

PipelineBase::~PipelineBase() {
    destroy();
}

void PipelineBase::destroy() {
    if (!m_initialized) return;
    
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    
    m_initialized = false;
}

} // namespace vk_core