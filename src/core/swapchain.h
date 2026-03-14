#pragma once

#include "device.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace vk_core {

class Swapchain {
public:
    Swapchain() = default;
    ~Swapchain();
    
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain(Swapchain&&) noexcept;
    Swapchain& operator=(Swapchain&&) noexcept;
    
    bool init(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
              uint32_t width, uint32_t height, bool vsync = true);
    void destroy();
    
    VkSwapchainKHR get() const { return m_swapchain; }
    VkFormat getImageFormat() const { return m_imageFormat; }
    VkExtent2D getExtent() const { return m_extent; }
    const std::vector<VkImage>& getImages() const { return m_images; }
    const std::vector<VkImageView>& getImageViews() const { return m_imageViews; }
    uint32_t getImageCount() const { return static_cast<uint32_t>(m_images.size()); }
    
    uint32_t acquireNextImage(VkSemaphore semaphore, VkFence fence = VK_NULL_HANDLE) const;
    void present(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore) const;
    
    bool recreate(uint32_t width, uint32_t height);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    
    VkFormat m_imageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_extent = {0, 0};
    
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
    
    bool m_vsync = true;
    bool m_initialized = false;
    
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) const;
    void createImageViews();
};

} // namespace vk_core