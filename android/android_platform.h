#pragma once

#include <vulkan/vulkan.h>
#include <android/native_window.h>
#include <string>

namespace vk_android {

bool initVulkan();
void cleanupVulkan();

VkInstance getInstance();
VkPhysicalDevice getPhysicalDevice();
VkDevice getDevice();
VkQueue getGraphicsQueue();
uint32_t getGraphicsQueueFamily();
VkCommandPool getCommandPool();

bool createSurface(ANativeWindow* window, VkSurfaceKHR* surface);
void destroySurface(VkSurfaceKHR surface);

std::string getExternalStoragePath();
std::string getCachePath();

}