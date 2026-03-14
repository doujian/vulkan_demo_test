#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace vk_core {

class Command {
public:
    Command() = default;
    ~Command();
    
    Command(const Command&) = delete;
    Command& operator=(const Command&) = delete;
    Command(Command&&) noexcept;
    Command& operator=(Command&&) noexcept;
    
    bool init(VkDevice device, uint32_t graphicsQueueFamily, uint32_t poolSize = 16);
    void destroy();
    
    VkCommandPool getPool() const { return m_commandPool; }
    VkDevice getDevice() const { return m_device; }
    
    VkCommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue) const;
    
    VkCommandBuffer allocateCommandBuffer() const;
    void freeCommandBuffer(VkCommandBuffer cmd) const;
    std::vector<VkCommandBuffer> allocateCommandBuffers(uint32_t count) const;
    void freeCommandBuffers(const std::vector<VkCommandBuffer>& cmdBuffers) const;
    
    void transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                               VkImageLayout oldLayout, VkImageLayout newLayout,
                               VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                               VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                               uint32_t mipLevels = 1, uint32_t layerCount = 1) const;
    
    void copyBuffer(VkCommandBuffer cmd, VkBuffer srcBuffer, VkBuffer dstBuffer,
                    VkDeviceSize size) const;
    void copyBufferToImage(VkCommandBuffer cmd, VkBuffer buffer, VkImage image,
                           uint32_t width, uint32_t height) const;

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    uint32_t m_queueFamily = 0;
    bool m_initialized = false;
};

} // namespace vk_core