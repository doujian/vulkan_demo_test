#pragma once

#include "instance.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace vk_core {

class Device {
public:
    Device() = default;
    ~Device();
    
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) noexcept;
    Device& operator=(Device&&) noexcept;
    
    bool init(VkInstance instance, VkSurfaceKHR surface = VK_NULL_HANDLE,
              const std::vector<const char*>& deviceExtensions = {});
    void destroy();
    
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    VkDevice get() const { return m_device; }
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getPresentQueue() const { return m_presentQueue; }
    VkQueue getComputeQueue() const { return m_computeQueue; }
    VkQueue getTransferQueue() const { return m_transferQueue; }
    
    uint32_t getGraphicsQueueFamily() const { return m_queueIndices.graphicsFamily.value(); }
    uint32_t getPresentQueueFamily() const { return m_queueIndices.presentFamily.value(); }
    uint32_t getComputeQueueFamily() const { return m_computeFamily.value(); }
    uint32_t getTransferQueueFamily() const { return m_transferFamily.value(); }
    
    const QueueFamilyIndices& getQueueIndices() const { return m_queueIndices; }
    
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                             VkImageTiling tiling, VkFormatFeatureFlags features) const;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    
    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    static bool createBufferWithMemory(VkDevice device, VkPhysicalDevice physicalDevice,
                                       const VkBufferCreateInfo* createInfo, VkMemoryPropertyFlags properties,
                                       VkBuffer* outBuffer, VkDeviceMemory* outMemory);
    static bool createImageWithMemory(VkDevice device, VkPhysicalDevice physicalDevice,
                                      const VkImageCreateInfo* createInfo, VkMemoryPropertyFlags properties,
                                      VkImage* outImage, VkDeviceMemory* outMemory);
    
    void waitIdle() const;
    
    VkPhysicalDeviceProperties getProperties() const;
    VkPhysicalDeviceFeatures getFeatures() const;

private:
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    VkQueue m_transferQueue = VK_NULL_HANDLE;
    
    QueueFamilyIndices m_queueIndices;
    std::optional<uint32_t> m_computeFamily;
    std::optional<uint32_t> m_transferFamily;
    
    VkPhysicalDeviceMemoryProperties m_memoryProperties{};
    
    bool m_initialized = false;
    
    bool pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    bool createLogicalDevice(VkSurfaceKHR surface, const std::vector<const char*>& extensions);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& extensions) const;
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const;
};

} // namespace vk_core