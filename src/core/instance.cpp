#include "instance.h"
#include "utils/validation_logger.h"
#include <iostream>
#include <cstring>

#ifdef VK_USE_PLATFORM_ANDROID_KHR
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "VulkanDemo", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "VulkanDemo", __VA_ARGS__)
#else
#include <GLFW/glfw3.h>
#endif

namespace vk_core {

namespace {
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
        
        std::string severityStr;
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            severityStr = "[ERROR] ";
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            severityStr = "[WARNING] ";
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            severityStr = "[INFO] ";
        } else {
            severityStr = "[VERBOSE] ";
        }
        
        std::string fullMessage = severityStr + std::string(pCallbackData->pMessage);
        
        vk_demo::ValidationLogger::instance().log(fullMessage);
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
        
        return VK_FALSE;
    }
}

Instance::Instance(Instance&& other) noexcept
    : m_instance(other.m_instance)
    , m_debugMessenger(other.m_debugMessenger)
    , m_validationEnabled(other.m_validationEnabled)
    , m_initialized(other.m_initialized) {
    other.m_instance = VK_NULL_HANDLE;
    other.m_debugMessenger = VK_NULL_HANDLE;
    other.m_initialized = false;
}

Instance& Instance::operator=(Instance&& other) noexcept {
    if (this != &other) {
        destroy();
        m_instance = other.m_instance;
        m_debugMessenger = other.m_debugMessenger;
        m_validationEnabled = other.m_validationEnabled;
        m_initialized = other.m_initialized;
        other.m_instance = VK_NULL_HANDLE;
        other.m_debugMessenger = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

Instance::~Instance() {
    destroy();
}

bool Instance::init(const std::vector<const char*>& extensions,
                    const std::vector<const char*>& layers,
                    bool enableValidation) {
    if (m_initialized) {
        return true;
    }

    m_validationEnabled = enableValidation;

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Demo Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto allExtensions = extensions;
    auto requiredExt = getRequiredExtensions();
    allExtensions.insert(allExtensions.end(), requiredExt.begin(), requiredExt.end());

    createInfo.enabledExtensionCount = static_cast<uint32_t>(allExtensions.size());
    createInfo.ppEnabledExtensionNames = allExtensions.data();

    std::vector<const char*> enabledLayers;
    if (enableValidation) {
        if (checkValidationLayerSupport(layers)) {
            enabledLayers = layers;
            if (enabledLayers.empty()) {
                enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
            }
        } else {
            std::cerr << "Validation layers requested, but not available!" << std::endl;
        }
    }

    createInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    createInfo.ppEnabledLayerNames = enabledLayers.data();

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance!" << std::endl;
        return false;
    }

    if (enableValidation) {
        setupDebugMessenger();
    }

    m_initialized = true;
    return true;
}

void Instance::destroy() {
    if (!m_initialized) return;

    if (m_debugMessenger != VK_NULL_HANDLE) {
        destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        m_debugMessenger = VK_NULL_HANDLE;
    }

    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }

    m_initialized = false;
}

bool Instance::checkValidationLayerSupport(const std::vector<const char*>& layers) const {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : layers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) return false;
    }
    return true;
}

std::vector<const char*> Instance::getRequiredExtensions() const {
    std::vector<const char*> extensions;
    
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#else
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        extensions.push_back(glfwExtensions[i]);
    }
#endif

    if (m_validationEnabled) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void Instance::setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    if (createDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
        std::cerr << "Failed to set up debug messenger!" << std::endl;
    }
}

std::vector<VkPhysicalDevice> Instance::enumeratePhysicalDevices() const {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    
    return devices;
}

VkResult Instance::createDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
    
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void Instance::destroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator) {
    
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

} // namespace vk_core