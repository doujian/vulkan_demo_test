#include "device.h"
#include <iostream>
#include <set>

namespace vk_core {

Device::Device(Device&& other) noexcept
    : m_physicalDevice(other.m_physicalDevice)
    , m_device(other.m_device)
    , m_graphicsQueue(other.m_graphicsQueue)
    , m_presentQueue(other.m_presentQueue)
    , m_computeQueue(other.m_computeQueue)
    , m_transferQueue(other.m_transferQueue)
    , m_queueIndices(other.m_queueIndices)
    , m_computeFamily(other.m_computeFamily)
    , m_transferFamily(other.m_transferFamily)
    , m_initialized(other.m_initialized) {
    other.m_device = VK_NULL_HANDLE;
    other.m_initialized = false;
}

Device& Device::operator=(Device&& other) noexcept {
    if (this != &other) {
        destroy();
        m_physicalDevice = other.m_physicalDevice;
        m_device = other.m_device;
        m_graphicsQueue = other.m_graphicsQueue;
        m_presentQueue = other.m_presentQueue;
        m_computeQueue = other.m_computeQueue;
        m_transferQueue = other.m_transferQueue;
        m_queueIndices = other.m_queueIndices;
        m_computeFamily = other.m_computeFamily;
        m_transferFamily = other.m_transferFamily;
        m_initialized = other.m_initialized;
        other.m_device = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

Device::~Device() {
    destroy();
}

bool Device::init(VkInstance instance, VkSurfaceKHR surface,
                  const std::vector<const char*>& deviceExtensions) {
    if (m_initialized) return true;
    
    if (!pickPhysicalDevice(instance, surface)) {
        return false;
    }
    
    if (!createLogicalDevice(surface, deviceExtensions)) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void Device::destroy() {
    if (!m_initialized) return;
    
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }
    
    m_initialized = false;
}

bool Device::pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        std::cerr << "Failed to find GPUs with Vulkan support!" << std::endl;
        return false;
    }
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    
    for (const auto& device : devices) {
        if (isDeviceSuitable(device, surface)) {
            m_physicalDevice = device;
            m_queueIndices = findQueueFamilies(device, surface);
            vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);
            break;
        }
    }
    
    if (m_physicalDevice == VK_NULL_HANDLE) {
        std::cerr << "Failed to find a suitable GPU!" << std::endl;
        return false;
    }
    
    return true;
}

bool Device::createLogicalDevice(VkSurfaceKHR surface, const std::vector<const char*>& extensions) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies;
    
    if (m_queueIndices.graphicsFamily.has_value()) {
        uniqueQueueFamilies.insert(m_queueIndices.graphicsFamily.value());
    }
    if (m_queueIndices.presentFamily.has_value()) {
        uniqueQueueFamilies.insert(m_queueIndices.presentFamily.value());
    }
    
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    
    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
        std::cerr << "Failed to create logical device!" << std::endl;
        return false;
    }
    
    vkGetDeviceQueue(m_device, m_queueIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
    if (m_queueIndices.presentFamily.has_value()) {
        vkGetDeviceQueue(m_device, m_queueIndices.presentFamily.value(), 0, &m_presentQueue);
    }
    
    m_computeFamily = m_queueIndices.graphicsFamily;
    m_transferFamily = m_queueIndices.graphicsFamily;
    m_computeQueue = m_graphicsQueue;
    m_transferQueue = m_graphicsQueue;
    
    return true;
}

QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const {
    QueueFamilyIndices indices;
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        
        if (surface != VK_NULL_HANDLE) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }
        }
        
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            indices.computeFamily = i;
        }
        
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            indices.transferFamily = i;
        }
        
        if (indices.isComplete(surface != VK_NULL_HANDLE)) break;
        i++;
    }
    
    return indices;
}

bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& extensions) const {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
    std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());
    
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    
    return requiredExtensions.empty();
}

bool Device::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const {
    bool needPresent = (surface != VK_NULL_HANDLE);
    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    
    bool extensionsSupported = true;
    
    bool swapchainAdequate = true;
    if (surface != VK_NULL_HANDLE) {
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        swapchainAdequate = (formatCount > 0) && (presentModeCount > 0);
    }
    
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    
    return indices.isComplete(needPresent) && extensionsSupported && swapchainAdequate && supportedFeatures.samplerAnisotropy;
}

VkFormat Device::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                     VkImageTiling tiling, VkFormatFeatureFlags features) const {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    return VK_FORMAT_UNDEFINED;
}

uint32_t Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}

uint32_t Device::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}

bool Device::createBufferWithMemory(VkDevice device, VkPhysicalDevice physicalDevice,
                                    const VkBufferCreateInfo* createInfo, VkMemoryPropertyFlags properties,
                                    VkBuffer* outBuffer, VkDeviceMemory* outMemory) {
    if (vkCreateBuffer(device, createInfo, nullptr, outBuffer) != VK_SUCCESS) {
        return false;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, *outBuffer, &memRequirements);
    
    uint32_t memoryTypeIndex;
    try {
        memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
    } catch (...) {
        vkDestroyBuffer(device, *outBuffer, nullptr);
        *outBuffer = VK_NULL_HANDLE;
        return false;
    }
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, outMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, *outBuffer, nullptr);
        *outBuffer = VK_NULL_HANDLE;
        return false;
    }
    
    vkBindBufferMemory(device, *outBuffer, *outMemory, 0);
    return true;
}

bool Device::createImageWithMemory(VkDevice device, VkPhysicalDevice physicalDevice,
                                   const VkImageCreateInfo* createInfo, VkMemoryPropertyFlags properties,
                                   VkImage* outImage, VkDeviceMemory* outMemory) {
    if (vkCreateImage(device, createInfo, nullptr, outImage) != VK_SUCCESS) {
        return false;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, *outImage, &memRequirements);
    
    uint32_t memoryTypeIndex;
    try {
        memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
    } catch (...) {
        vkDestroyImage(device, *outImage, nullptr);
        *outImage = VK_NULL_HANDLE;
        return false;
    }
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, outMemory) != VK_SUCCESS) {
        vkDestroyImage(device, *outImage, nullptr);
        *outImage = VK_NULL_HANDLE;
        return false;
    }
    
    vkBindImageMemory(device, *outImage, *outMemory, 0);
    return true;
}

void Device::waitIdle() const {
    vkDeviceWaitIdle(m_device);
}

VkPhysicalDeviceProperties Device::getProperties() const {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
    return props;
}

VkPhysicalDeviceFeatures Device::getFeatures() const {
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(m_physicalDevice, &features);
    return features;
}

} // namespace vk_core