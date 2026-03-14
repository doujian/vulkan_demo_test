#include "sync.h"

namespace vk_core {

Sync::Sync(Sync&& other) noexcept
    : m_device(other.m_device)
    , m_imageAvailableSemaphores(std::move(other.m_imageAvailableSemaphores))
    , m_renderFinishedSemaphores(std::move(other.m_renderFinishedSemaphores))
    , m_inFlightFences(std::move(other.m_inFlightFences))
    , m_frameCount(other.m_frameCount)
    , m_initialized(other.m_initialized) {
    other.m_initialized = false;
}

Sync& Sync::operator=(Sync&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_imageAvailableSemaphores = std::move(other.m_imageAvailableSemaphores);
        m_renderFinishedSemaphores = std::move(other.m_renderFinishedSemaphores);
        m_inFlightFences = std::move(other.m_inFlightFences);
        m_frameCount = other.m_frameCount;
        m_initialized = other.m_initialized;
        other.m_initialized = false;
    }
    return *this;
}

Sync::~Sync() {
    destroy();
}

bool Sync::init(VkDevice device, uint32_t frameCount) {
    if (m_initialized) return true;
    
    m_device = device;
    m_frameCount = frameCount;
    
    m_imageAvailableSemaphores.resize(frameCount);
    m_renderFinishedSemaphores.resize(frameCount);
    m_inFlightFences.resize(frameCount);
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (uint32_t i = 0; i < frameCount; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            return false;
        }
    }
    
    m_initialized = true;
    return true;
}

void Sync::destroy() {
    if (!m_initialized) return;
    
    for (auto sem : m_imageAvailableSemaphores) {
        if (sem != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, sem, nullptr);
        }
    }
    
    for (auto sem : m_renderFinishedSemaphores) {
        if (sem != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, sem, nullptr);
        }
    }
    
    for (auto fence : m_inFlightFences) {
        if (fence != VK_NULL_HANDLE) {
            vkDestroyFence(m_device, fence, nullptr);
        }
    }
    
    m_imageAvailableSemaphores.clear();
    m_renderFinishedSemaphores.clear();
    m_inFlightFences.clear();
    
    m_initialized = false;
}

VkSemaphore Sync::getImageAvailableSemaphore(size_t frame) const {
    return frame < m_imageAvailableSemaphores.size() ? m_imageAvailableSemaphores[frame] : VK_NULL_HANDLE;
}

VkSemaphore Sync::getRenderFinishedSemaphore(size_t frame) const {
    return frame < m_renderFinishedSemaphores.size() ? m_renderFinishedSemaphores[frame] : VK_NULL_HANDLE;
}

VkFence Sync::getInFlightFence(size_t frame) const {
    return frame < m_inFlightFences.size() ? m_inFlightFences[frame] : VK_NULL_HANDLE;
}

void Sync::waitFence(size_t frame) const {
    if (frame < m_inFlightFences.size()) {
        vkWaitForFences(m_device, 1, &m_inFlightFences[frame], VK_TRUE, UINT64_MAX);
    }
}

void Sync::resetFence(size_t frame) const {
    if (frame < m_inFlightFences.size()) {
        vkResetFences(m_device, 1, &m_inFlightFences[frame]);
    }
}

} // namespace vk_core