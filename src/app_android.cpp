#include "app.h"
#include "demo/demo_registry.h"
#include "core/image.h"
#include "utils/png_writer.h"
#include "utils/validation_logger.h"
#include "test/test_runner.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cstring>

#include <android/log.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "VulkanDemo", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "VulkanDemo", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "VulkanDemo", __VA_ARGS__)

namespace vk_demo {

namespace {
    std::string g_outputPath;
    std::string g_goldenPath;
}

App::~App() {
    cleanup();
}

int App::run(int argc, char* argv[]) {
    forceLinkDemos();
    ValidationLogger::instance().clear();
    
    auto args = vk_utils::ArgsParser::parse(argc, argv);
    m_validationEnabled = args.validation;
    m_testMode = args.testMode;
    m_updateGolden = args.updateGolden;
    m_goldenPath = args.goldenPath;
    m_testThreshold = args.testThreshold;
    
    if (args.outputPath.empty()) {
        args.outputPath = "/sdcard/vulkan_demo_output";
    }
    if (args.goldenPath.empty()) {
        args.goldenPath = "/data/local/tmp/golden";
    }
    
    g_outputPath = args.outputPath;
    g_goldenPath = args.goldenPath;
    
    mkdir(args.outputPath.c_str(), 0755);
    mkdir(args.goldenPath.c_str(), 0755);
    
    if (args.listDemos) {
        LOGI("Available demos:");
        for (const auto& name : DemoRegistry::instance().getDemoNames()) {
            LOGI("  %s: %s", name.c_str(), DemoRegistry::instance().getDescription(name).c_str());
        }
        return 0;
    }
    
    if (args.runAllTests) {
        return runAllDemosTests(args);
    }
    
    auto demo = DemoRegistry::instance().createDemo(args.demoName);
    if (!demo) {
        LOGE("Demo '%s' not found. Use -l to see available demos.", args.demoName.c_str());
        return 1;
    }
    
    args.offscreen = true;
    
    if (!init(args, demo)) {
        return 1;
    }
    
    int exitCode = 0;
    std::string outputDir = runDemo(demo, args, exitCode);
    
    if (m_validationEnabled && !outputDir.empty() && ValidationLogger::instance().hasMessages()) {
        std::string logFile = outputDir + "/validation.txt";
        if (ValidationLogger::instance().saveToFile(logFile)) {
            LOGI("Validation log saved to: %s", logFile.c_str());
        }
    }
    
    demo->destroy();
    cleanup();
    return exitCode;
}

bool App::init(const vk_utils::Args& args, std::unique_ptr<DemoBase>& demo) {
    if (!initVulkan(args.validation)) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void App::cleanup() {
    if (!m_initialized) return;
    
    m_command.destroy();
    m_device.destroy();
    
    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance.get(), m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
    
    m_instance.destroy();
    m_initialized = false;
}

bool App::initVulkan(bool enableValidation) {
    if (!m_instance.init({}, {}, enableValidation)) {
        LOGE("Failed to create Vulkan instance.");
        return false;
    }
    
    std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    if (!m_device.init(m_instance.get(), VK_NULL_HANDLE, extensions)) {
        LOGE("Failed to create device.");
        return false;
    }
    
    if (!m_command.init(m_device.get(), m_device.getGraphicsQueueFamily())) {
        LOGE("Failed to create command pool.");
        return false;
    }
    
    LOGI("Vulkan initialized successfully");
    return true;
}

std::string App::createOutputDirectory(const std::string& demoName, const std::string& basePath) {
    auto now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    std::ostringstream ss;
    ss << std::put_time(tm, "%Y%m%d_%H%M%S");
    
    std::string dir = basePath + "/" + demoName + "_" + ss.str();
    
    mkdir(basePath.c_str(), 0755);
    mkdir(dir.c_str(), 0755);
    
    return dir;
}

DemoContext App::createDemoContext(bool isWindowMode) {
    DemoContext context{};
    context.instance = m_instance.get();
    context.physicalDevice = m_device.getPhysicalDevice();
    context.device = m_device.get();
    context.graphicsQueue = m_device.getGraphicsQueue();
    context.presentQueue = m_device.getGraphicsQueue();
    context.commandPool = m_command.getPool();
    context.extent = {800, 600};
    context.colorFormat = VK_FORMAT_R8G8B8A8_SRGB;
    context.graphicsQueueFamily = m_device.getGraphicsQueueFamily();
    context.presentQueueFamily = m_device.getGraphicsQueueFamily();
    return context;
}

bool App::renderOffscreenFrame(DemoBase* demo, const std::string& outputDir, uint32_t frameCount,
                                vk_test::TestRunner& testRunner, uint32_t& passCount, uint32_t& failCount) {
    VkCommandBuffer cmd = m_command.allocateCommandBuffer();
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);
    
    demo->recordCommandBuffer(cmd);
    
    vkEndCommandBuffer(cmd);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    
    vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_device.getGraphicsQueue());
    
    VkImage outputImage = demo->getOutputImage();
    VkExtent2D outputExtent = demo->getOutputExtent();
    VkFormat outputFormat = demo->getOutputFormat();
    
    char filename[512];
    snprintf(filename, sizeof(filename), "%s/%04u.png", outputDir.c_str(), frameCount);
    
    vk_utils::SaveImageInfo saveInfo{};
    saveInfo.device = m_device.get();
    saveInfo.physicalDevice = m_device.getPhysicalDevice();
    saveInfo.queue = m_device.getGraphicsQueue();
    saveInfo.commandPool = m_command.getPool();
    saveInfo.image = outputImage;
    saveInfo.format = outputFormat;
    saveInfo.width = outputExtent.width;
    saveInfo.height = outputExtent.height;
    saveInfo.filename = filename;
    
    if (!vk_utils::saveVkImageToPNG(saveInfo)) {
        LOGE("Failed to save: %s", filename);
        m_command.freeCommandBuffer(cmd);
        return false;
    }
    
    if (m_updateGolden) {
        testRunner.updateGolden(demo->getName(), filename, frameCount);
        LOGI("Frame %u: Golden updated", frameCount);
    } else if (m_testMode) {
        vk_test::TestResult result = testRunner.runTest(demo->getName(),
            [&filename]() { return filename; }, frameCount, m_testThreshold);
        
        LOGI("[%s] %s", result.testName.c_str(), result.message.c_str());
        
        if (result.passed) {
            passCount++;
        } else {
            failCount++;
        }
    } else {
        LOGI("Saved: %s", filename);
    }
    
    m_command.freeCommandBuffer(cmd);
    return true;
}

void App::printTestSummary(uint32_t passCount, uint32_t failCount, uint32_t frameCount, bool isTestMode) {
    if (isTestMode) {
        LOGI("=== Test Summary ===");
        LOGI("Passed: %u/%u", passCount, frameCount);
        LOGI("Failed: %u/%u", failCount, frameCount);
    }
}

std::string App::runDemo(std::unique_ptr<DemoBase>& demo, const vk_utils::Args& args, int& exitCode) {
    exitCode = 0;
    bool isWindowMode = false;
    uint32_t maxFrames = args.frameCount;
    
    std::string outputDir;
    if (m_testMode || m_updateGolden) {
        outputDir = args.outputPath + "/" + demo->getName();
        mkdir(outputDir.c_str(), 0755);
    } else {
        outputDir = createOutputDirectory(demo->getName(), args.outputPath);
        LOGI("Output: %s", outputDir.c_str());
        LOGI("Rendering %u frames...", maxFrames > 0 ? maxFrames : 1);
    }
    
    DemoContext context = createDemoContext(isWindowMode);
    
    if (!demo->init(context)) {
        LOGE("Failed to initialize demo.");
        exitCode = 1;
        return outputDir;
    }
    
    vk_test::TestRunner testRunner(m_goldenPath, outputDir);
    uint32_t passCount = 0;
    uint32_t failCount = 0;
    uint32_t frameCount = 0;
    
    uint32_t targetFrames = maxFrames > 0 ? maxFrames : 1;
    
    while (frameCount < targetFrames) {
        renderOffscreenFrame(demo.get(), outputDir, frameCount, testRunner, passCount, failCount);
        frameCount++;
        demo->update(0.016f);
    }
    
    m_device.waitIdle();
    
    if (m_testMode) {
        printTestSummary(passCount, failCount, frameCount, true);
        exitCode = (failCount > 0) ? 1 : 0;
    } else if (!m_updateGolden) {
        LOGI("Done. %u frames saved to %s", frameCount, outputDir.c_str());
    }
    
    return outputDir;
}

int App::runAllDemosTests(const vk_utils::Args& args) {
    if (!initVulkan(args.validation)) {
        return 1;
    }
    
    m_initialized = true;
    
    auto demoNames = DemoRegistry::instance().getDemoNames();
    if (demoNames.empty()) {
        LOGE("No demos available.");
        return 1;
    }
    
    uint32_t totalPass = 0;
    uint32_t totalFail = 0;
    std::vector<std::string> failedDemos;
    
    LOGI("=== Running all demo tests ===");
    LOGI("Total demos: %zu", demoNames.size());
    
    mkdir(args.outputPath.c_str(), 0755);
    mkdir(args.goldenPath.c_str(), 0755);
    
    for (const auto& demoName : demoNames) {
        LOGI("--- Testing demo: %s ---", demoName.c_str());
        
        auto demo = DemoRegistry::instance().createDemo(demoName);
        if (!demo) {
            LOGE("Failed to create demo: %s", demoName.c_str());
            totalFail++;
            failedDemos.push_back(demoName);
            continue;
        }
        
        std::string outputDir = args.outputPath + "/" + demoName;
        mkdir(outputDir.c_str(), 0755);
        
        DemoContext context = createDemoContext(false);
        
        if (!demo->init(context)) {
            LOGE("Failed to initialize demo: %s", demoName.c_str());
            totalFail++;
            failedDemos.push_back(demoName);
            continue;
        }
        
        vk_test::TestRunner testRunner(args.goldenPath, outputDir);
        uint32_t passCount = 0;
        uint32_t failCount = 0;
        
        uint32_t frames = args.frameCount > 0 ? args.frameCount : 1;
        
        for (uint32_t i = 0; i < frames; i++) {
            VkCommandBuffer cmd = m_command.allocateCommandBuffer();
            
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            vkBeginCommandBuffer(cmd, &beginInfo);
            
            demo->recordCommandBuffer(cmd);
            
            vkEndCommandBuffer(cmd);
            
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmd;
            
            vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(m_device.getGraphicsQueue());
            
            m_command.freeCommandBuffer(cmd);
            
            VkImage outputImage = demo->getOutputImage();
            VkExtent2D outputExtent = demo->getOutputExtent();
            
            char filename[512];
            snprintf(filename, sizeof(filename), "%s/%04u.png", outputDir.c_str(), i);
            
            vk_utils::SaveImageInfo saveInfo{};
            saveInfo.device = m_device.get();
            saveInfo.physicalDevice = m_device.getPhysicalDevice();
            saveInfo.queue = m_device.getGraphicsQueue();
            saveInfo.commandPool = m_command.getPool();
            saveInfo.image = outputImage;
            saveInfo.format = context.colorFormat;
            saveInfo.width = outputExtent.width;
            saveInfo.height = outputExtent.height;
            saveInfo.filename = filename;
            
            if (!vk_utils::saveVkImageToPNG(saveInfo)) {
                LOGE("Failed to save: %s", filename);
                continue;
            }
            
            if (args.updateGolden) {
                testRunner.updateGolden(demoName, filename, i);
                LOGI("  Frame %u: Golden updated", i);
            } else {
                vk_test::TestResult result = testRunner.runTest(demoName,
                    [&filename]() { return filename; }, i, args.testThreshold);
                
                LOGI("  Frame %u: %s", i, result.message.c_str());
                
                if (result.passed) {
                    passCount++;
                } else {
                    failCount++;
                }
            }
        }
        
        demo->destroy();
        
        if (!args.updateGolden) {
            if (failCount == 0) {
                LOGI("  [PASS] %s: %u/%u frames passed", demoName.c_str(), passCount, frames);
                totalPass++;
            } else {
                LOGI("  [FAIL] %s: %u/%u frames passed", demoName.c_str(), passCount, frames);
                totalFail++;
                failedDemos.push_back(demoName);
            }
        }
    }
    
    LOGI("=== Test Summary ===");
    if (args.updateGolden) {
        LOGI("Golden images updated for all demos.");
    } else {
        LOGI("Passed: %u/%zu demos", totalPass, demoNames.size());
        LOGI("Failed: %u/%zu demos", totalFail, demoNames.size());
        
        if (!failedDemos.empty()) {
            LOGI("Failed demos:");
            for (const auto& name : failedDemos) {
                LOGI("  - %s", name.c_str());
            }
        }
    }
    
    cleanup();
    return args.updateGolden ? 0 : (totalFail > 0 ? 1 : 0);
}

bool App::initWindow(uint32_t width, uint32_t height) {
    return true;
}

bool App::initWindowModeResources(WindowModeResources& res) {
    return true;
}

void App::cleanupWindowModeResources(WindowModeResources& res) {
}

bool App::renderWindowFrame(WindowModeResources& res, DemoBase* demo, size_t frameIndex) {
    return true;
}

}