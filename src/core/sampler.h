#pragma once

#include <vulkan/vulkan.h>

namespace vk_core {

class Sampler {
public:
    Sampler() = default;
    ~Sampler();
    
    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;
    Sampler(Sampler&&) noexcept;
    Sampler& operator=(Sampler&&) noexcept;
    
    bool init(VkDevice device, VkFilter filter = VK_FILTER_LINEAR,
              VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    void destroy();
    
    VkSampler get() const { return m_sampler; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
    bool m_initialized = false;
};

} // namespace vk_core