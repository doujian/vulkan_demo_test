#include "buffer.h"
#include "device.h"
#include "command.h"
#include <cstring>

namespace vk_core {

Buffer::Buffer(Buffer&& other) noexcept
    : m_device(other.m_device)
    , m_buffer(other.m_buffer)
    , m_memory(other.m_memory)
    , m_size(other.m_size)
    , m_initialized(other.m_initialized) {
    other.m_buffer = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_initialized = false;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_buffer = other.m_buffer;
        m_memory = other.m_memory;
        m_size = other.m_size;
        m_initialized = other.m_initialized;
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

Buffer::~Buffer() {
    destroy();
}

bool Buffer::init(VkDevice device, VkPhysicalDevice physicalDevice,
                  VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties) {
    if (m_initialized) return true;
    
    m_device = device;
    m_size = size;
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (!Device::createBufferWithMemory(device, physicalDevice, &bufferInfo, properties, &m_buffer, &m_memory)) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void Buffer::destroy() {
    if (!m_initialized) return;
    
    if (m_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_buffer, nullptr);
        m_buffer = VK_NULL_HANDLE;
    }
    
    if (m_memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_memory, nullptr);
        m_memory = VK_NULL_HANDLE;
    }
    
    m_initialized = false;
}

void* Buffer::map() const {
    void* data = nullptr;
    VkResult result = vkMapMemory(m_device, m_memory, 0, m_size, 0, &data);
    if (result != VK_SUCCESS) {
        return nullptr;
    }
    return data;
}

void Buffer::unmap() const {
    vkUnmapMemory(m_device, m_memory);
}

void Buffer::setData(const void* data, VkDeviceSize size) const {
    void* mapped = map();
    memcpy(mapped, data, static_cast<size_t>(size));
    unmap();
}

void Buffer::copy(const BufferCopyInfo& info) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = info.cmdPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(info.device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    VkBufferCopy copyRegion{};
    copyRegion.size = info.size;
    vkCmdCopyBuffer(commandBuffer, info.srcBuffer, info.dstBuffer, 1, &copyRegion);
    
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(info.queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(info.queue);
    
    vkFreeCommandBuffers(info.device, info.cmdPool, 1, &commandBuffer);
}

void VertexBuffer::bind(VkCommandBuffer cmd, uint32_t binding) const {
    VkBuffer vertexBuffers[] = { get() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, binding, 1, vertexBuffers, offsets);
}

void IndexBuffer::bind(VkCommandBuffer cmd) const {
    vkCmdBindIndexBuffer(cmd, get(), 0, VK_INDEX_TYPE_UINT32);
}

VkDescriptorBufferInfo UniformBuffer::getDescriptorInfo() const {
    VkDescriptorBufferInfo info{};
    info.buffer = get();
    info.offset = 0;
    info.range = getSize();
    return info;
}

} // namespace vk_core