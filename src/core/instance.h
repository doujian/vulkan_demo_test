#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <optional>
#include <functional>

namespace vk_core {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;
    
    bool isComplete(bool needPresent = true) const {
        if (needPresent) {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
        return graphicsFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Instance {
public:
    Instance() = default;
    ~Instance();
    
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;
    Instance(Instance&&) noexcept;
    Instance& operator=(Instance&&) noexcept;
    
    bool init(const std::vector<const char*>& extensions = {}, 
              const std::vector<const char*>& layers = {},
              bool enableValidation = true);
    
    void destroy();
    
    VkInstance get() const { return m_instance; }
    VkDebugUtilsMessengerEXT getDebugMessenger() const { return m_debugMessenger; }
    bool isValidationEnabled() const { return m_validationEnabled; }
    
    std::vector<VkPhysicalDevice> enumeratePhysicalDevices() const;
    std::vector<const char*> getRequiredExtensions() const;
    
    static VkResult createDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);
    
    static void destroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator);

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    bool m_validationEnabled = false;
    bool m_initialized = false;
    
    bool checkValidationLayerSupport(const std::vector<const char*>& layers) const;
    void setupDebugMessenger();
};

} // namespace vk_core