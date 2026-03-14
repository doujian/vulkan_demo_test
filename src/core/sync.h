#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace vk_core {

class Sync {
public:
    Sync() = default;
    ~Sync();
    
    Sync(const Sync&) = delete;
    Sync& operator=(const Sync&) = delete;
    Sync(Sync&&) noexcept;
    Sync& operator=(Sync&&) noexcept;
    
    bool init(VkDevice device, uint32_t frameCount = 2);
    void destroy();
    
    VkSemaphore getImageAvailableSemaphore(size_t frame) const;
    VkSemaphore getRenderFinishedSemaphore(size_t frame) const;
    VkFence getInFlightFence(size_t frame) const;
    
    void waitFence(size_t frame) const;
    void resetFence(size_t frame) const;
    
    uint32_t getFrameCount() const { return m_frameCount; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    uint32_t m_frameCount = 0;
    bool m_initialized = false;
};

} // namespace vk_core