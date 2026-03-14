#include "command.h"
#include <stdexcept>

namespace vk_core {

Command::Command(Command&& other) noexcept
    : m_device(other.m_device)
    , m_commandPool(other.m_commandPool)
    , m_queueFamily(other.m_queueFamily)
    , m_initialized(other.m_initialized) {
    other.m_commandPool = VK_NULL_HANDLE;
    other.m_initialized = false;
}

Command& Command::operator=(Command&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_commandPool = other.m_commandPool;
        m_queueFamily = other.m_queueFamily;
        m_initialized = other.m_initialized;
        other.m_commandPool = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

Command::~Command() {
    destroy();
}

bool Command::init(VkDevice device, uint32_t graphicsQueueFamily, uint32_t poolSize) {
    if (m_initialized) return true;
    
    m_device = device;
    m_queueFamily = graphicsQueueFamily;
    
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void Command::destroy() {
    if (!m_initialized) return;
    
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }
    
    m_initialized = false;
}

VkCommandBuffer Command::beginSingleTimeCommands() const {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    return commandBuffer;
}

void Command::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue) const {
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

VkCommandBuffer Command::allocateCommandBuffer() const {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);
    return commandBuffer;
}

void Command::freeCommandBuffer(VkCommandBuffer cmd) const {
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
}

std::vector<VkCommandBuffer> Command::allocateCommandBuffers(uint32_t count) const {
    std::vector<VkCommandBuffer> buffers(count);
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = count;
    
    vkAllocateCommandBuffers(m_device, &allocInfo, buffers.data());
    return buffers;
}

void Command::freeCommandBuffers(const std::vector<VkCommandBuffer>& cmdBuffers) const {
    vkFreeCommandBuffers(m_device, m_commandPool, 
                         static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());
}

void Command::transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                                    VkImageLayout oldLayout, VkImageLayout newLayout,
                                    VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                                    VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                                    uint32_t mipLevels, uint32_t layerCount) const {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;
    
    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void Command::copyBuffer(VkCommandBuffer cmd, VkBuffer srcBuffer, VkBuffer dstBuffer,
                         VkDeviceSize size) const {
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);
}

void Command::copyBufferToImage(VkCommandBuffer cmd, VkBuffer buffer, VkImage image,
                                uint32_t width, uint32_t height) const {
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    
    vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

} // namespace vk_core