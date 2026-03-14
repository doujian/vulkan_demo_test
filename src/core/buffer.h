#pragma once

#include <vulkan/vulkan.h>
#include "device.h"

namespace vk_core {

struct BufferCopyInfo {
    VkDevice device = VK_NULL_HANDLE;
    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkBuffer srcBuffer = VK_NULL_HANDLE;
    VkBuffer dstBuffer = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
};

class Buffer {
public:
    Buffer() = default;
    ~Buffer();
    
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) noexcept;
    Buffer& operator=(Buffer&&) noexcept;
    
    bool init(VkDevice device, VkPhysicalDevice physicalDevice,
              VkDeviceSize size, VkBufferUsageFlags usage,
              VkMemoryPropertyFlags properties);
    void destroy();
    
    VkBuffer get() const { return m_buffer; }
    VkDeviceMemory getMemory() const { return m_memory; }
    VkDeviceSize getSize() const { return m_size; }
    
    void* map() const;
    void unmap() const;
    void setData(const void* data, VkDeviceSize size) const;
    
    static void copy(const BufferCopyInfo& info);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_size = 0;
    bool m_initialized = false;
};

class VertexBuffer : public Buffer {
public:
    void bind(VkCommandBuffer cmd, uint32_t binding = 0) const;
};

class IndexBuffer : public Buffer {
public:
    void bind(VkCommandBuffer cmd) const;
    uint32_t getIndexCount() const { return m_indexCount; }
    void setIndexCount(uint32_t count) { m_indexCount = count; }
    
private:
    uint32_t m_indexCount = 0;
};

class UniformBuffer : public Buffer {
public:
    VkDescriptorBufferInfo getDescriptorInfo() const;
};

} // namespace vk_core