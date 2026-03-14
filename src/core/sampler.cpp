#include "sampler.h"

namespace vk_core {

Sampler::~Sampler() {
    destroy();
}

Sampler::Sampler(Sampler&& other) noexcept
    : m_device(other.m_device)
    , m_sampler(other.m_sampler)
    , m_initialized(other.m_initialized) {
    other.m_device = VK_NULL_HANDLE;
    other.m_sampler = VK_NULL_HANDLE;
    other.m_initialized = false;
}

Sampler& Sampler::operator=(Sampler&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_sampler = other.m_sampler;
        m_initialized = other.m_initialized;
        other.m_device = VK_NULL_HANDLE;
        other.m_sampler = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

bool Sampler::init(VkDevice device, VkFilter filter, VkSamplerAddressMode addressMode) {
    m_device = device;
    
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = filter;
    samplerInfo.minFilter = filter;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    if (vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void Sampler::destroy() {
    if (!m_initialized) return;
    
    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }
    
    m_initialized = false;
}

} // namespace vk_core