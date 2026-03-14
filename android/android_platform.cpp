#include "android_platform.h"
#include "core/instance.h"
#include "core/device.h"
#include "core/command.h"

#include <android/log.h>
#include <android/native_window.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "VulkanDemo", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "VulkanDemo", __VA_ARGS__)

namespace vk_android {

static vk_core::Instance g_instance;
static vk_core::Device g_device;
static vk_core::Command g_command;
static bool g_initialized = false;

bool initVulkan() {
    if (g_initialized) return true;
    
    if (!g_instance.init({}, {}, false)) {
        LOGE("Failed to create Vulkan instance");
        return false;
    }
    
    std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    if (!g_device.init(g_instance.get(), VK_NULL_HANDLE, extensions)) {
        LOGE("Failed to create device");
        return false;
    }
    
    if (!g_command.init(g_device.get(), g_device.getGraphicsQueueFamily())) {
        LOGE("Failed to create command pool");
        return false;
    }
    
    g_initialized = true;
    LOGI("Vulkan initialized successfully");
    return true;
}

void cleanupVulkan() {
    if (!g_initialized) return;
    
    g_command.destroy();
    g_device.destroy();
    g_instance.destroy();
    g_initialized = false;
}

VkInstance getInstance() {
    return g_instance.get();
}

VkPhysicalDevice getPhysicalDevice() {
    return g_device.getPhysicalDevice();
}

VkDevice getDevice() {
    return g_device.get();
}

VkQueue getGraphicsQueue() {
    return g_device.getGraphicsQueue();
}

uint32_t getGraphicsQueueFamily() {
    return g_device.getGraphicsQueueFamily();
}

VkCommandPool getCommandPool() {
    return g_command.getPool();
}

bool createSurface(ANativeWindow* window, VkSurfaceKHR* surface) {
    VkAndroidSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.window = window;
    
    VkResult result = vkCreateAndroidSurfaceKHR(g_instance.get(), &createInfo, nullptr, surface);
    if (result != VK_SUCCESS) {
        LOGE("Failed to create Android surface: %d", result);
        return false;
    }
    return true;
}

void destroySurface(VkSurfaceKHR surface) {
    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(g_instance.get(), surface, nullptr);
    }
}

std::string getExternalStoragePath() {
    const char* path = getenv("EXTERNAL_STORAGE");
    if (path) return std::string(path);
    return "/sdcard";
}

std::string getCachePath() {
    const char* path = getenv("CACHE_PATH");
    if (path) return std::string(path);
    return "/data/local/tmp";
}

}