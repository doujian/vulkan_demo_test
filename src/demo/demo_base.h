#pragma once

#include "demo_config.h"
#include <vulkan/vulkan.h>
#include <string>
#include <memory>

namespace vk_demo {

struct DemoContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkExtent2D extent = {800, 600};
    VkFormat colorFormat = VK_FORMAT_R8G8B8A8_SRGB;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    uint32_t graphicsQueueFamily = 0;
    uint32_t presentQueueFamily = 0;
};

class DemoBase {
public:
    virtual ~DemoBase() = default;
    
    virtual bool init(const DemoContext& context) = 0;
    virtual void destroy() = 0;
    
    virtual void recordCommandBuffer(VkCommandBuffer cmd) = 0;
    virtual void update(float deltaTime) {}
    
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const { return ""; }
    
    virtual bool needDepth() const { return true; }
    virtual DemoConfig getConfig() const { return getDefaultConfig(); }
    
    virtual VkImage getOutputImage() const = 0;
    virtual VkExtent2D getOutputExtent() const = 0;
    virtual VkFormat getOutputFormat() const { return VK_FORMAT_R8G8B8A8_SRGB; }
};

} // namespace vk_demo