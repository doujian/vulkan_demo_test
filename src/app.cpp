#include "app.h"
#include "demo/demo_registry.h"
#include "core/image.h"
#include "utils/png_writer.h"
#include "utils/validation_logger.h"
#include "utils/file_utils.h"
#include "test/test_runner.h"

#ifdef __ANDROID__
#include <android/log.h>
#include <sys/stat.h>
#include <unistd.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "VulkanDemo", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "VulkanDemo", __VA_ARGS__)
#else
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdio>
#define LOGI(...) do { printf(__VA_ARGS__); printf("\n"); fflush(stdout); } while(0)
#define LOGE(...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); } while(0)
#ifdef _WIN32
#include <direct.h>
#endif
#endif

#include <iomanip>
#include <sstream>
#include <ctime>

namespace vk_demo {

namespace {
#ifdef __ANDROID__
    std::string g_outputPath;
    std::string g_goldenPath;
#endif
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
    m_testThreshold = args.testThreshold;
    
#ifdef __ANDROID__
    if (args.outputPath.empty()) {
        args.outputPath = "/sdcard/vulkan_demo_output";
    }
    if (args.goldenPath.empty()) {
        args.goldenPath = "/data/local/tmp/golden";
    }
    g_outputPath = args.outputPath;
    g_goldenPath = args.goldenPath;
    args.offscreen = true;
#else
    m_goldenPath = vk_utils::resolveResourcePath(args.goldenPath);
#endif
    
    makeDirectory(args.outputPath);
    makeDirectory(args.goldenPath);
    
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
#ifndef __ANDROID__
    if (!initWindow(args.width, args.height)) {
        return false;
    }
#endif
    
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
    
#ifndef __ANDROID__
    if (m_window) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_window = nullptr;
    }
#endif
    
    m_initialized = false;
}

#ifndef __ANDROID__
bool App::initWindow(uint32_t width, uint32_t height) {
    glfwSetErrorCallback([](int error, const char* desc) {
        std::cerr << "GLFW Error " << error << ": " << desc << std::endl;
    });
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW.\n";
        return false;
    }
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    m_window = glfwCreateWindow(width, height, "Vulkan Demo", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create window.\n";
        glfwTerminate();
        return false;
    }
    
    return true;
}
#endif

bool App::initVulkan(bool enableValidation) {
    if (!m_instance.init({}, {}, enableValidation)) {
        LOGE("Failed to create Vulkan instance.");
        return false;
    }
    
#ifndef __ANDROID__
    if (glfwCreateWindowSurface(m_instance.get(), m_window, nullptr, &m_surface) != VK_SUCCESS) {
        std::cerr << "Failed to create surface.\n";
        return false;
    }
#endif
    
    std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    if (!m_device.init(m_instance.get(), m_surface, extensions)) {
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

void App::makeDirectory(const std::string& path) {
#ifdef _WIN32
    _mkdir(path.c_str());
#elif defined(__ANDROID__)
    mkdir(path.c_str(), 0755);
#else
    mkdir(path.c_str(), 0755);
#endif
}

std::string App::createOutputDirectory(const std::string& demoName, const std::string& basePath) {
    auto now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    std::ostringstream ss;
    ss << std::put_time(tm, "%Y%m%d_%H%M%S");
    
    std::string dir = basePath + "/" + demoName + "_" + ss.str();
    
    makeDirectory(basePath);
    makeDirectory(dir);
    
    return dir;
}

std::string App::setupOutputDirectory(const std::string& demoName, const vk_utils::Args& args, bool isWindowMode) {
    std::string outputDir;
    
    if (isWindowMode) {
        outputDir = createOutputDirectory(demoName, args.outputPath);
    } else if (m_testMode || m_updateGolden) {
        outputDir = args.outputPath + "/" + demoName;
        makeDirectory(args.outputPath);
        makeDirectory(outputDir);
    } else {
        outputDir = createOutputDirectory(demoName, args.outputPath);
        LOGI("Output: %s", outputDir.c_str());
        LOGI("Rendering %u frames...", args.frameCount > 0 ? args.frameCount : 1);
    }
    
    return outputDir;
}

DemoContext App::createDemoContext(bool isWindowMode) {
    DemoContext context{};
    context.instance = m_instance.get();
    context.physicalDevice = m_device.getPhysicalDevice();
    context.device = m_device.get();
    context.graphicsQueue = m_device.getGraphicsQueue();
    context.presentQueue = isWindowMode ? m_device.getPresentQueue() : m_device.getGraphicsQueue();
    context.commandPool = m_command.getPool();
    context.extent = {800, 600};
    context.colorFormat = isWindowMode ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_R8G8B8A8_SRGB;
    context.graphicsQueueFamily = m_device.getGraphicsQueueFamily();
    context.presentQueueFamily = isWindowMode ? m_device.getPresentQueueFamily() : m_device.getGraphicsQueueFamily();
    return context;
}

#ifndef __ANDROID__
bool App::initWindowModeResources(WindowModeResources& res) {
    res.swapchain.init(m_device.get(), m_device.getPhysicalDevice(), m_surface, 800, 600, true);
    res.renderPass.init(m_device.get(), res.swapchain.getImageFormat());
    res.sync.init(m_device.get(), 2);
    
    res.framebuffers.resize(res.swapchain.getImageCount());
    for (size_t i = 0; i < res.swapchain.getImageCount(); i++) {
        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = res.renderPass.get();
        info.attachmentCount = 1;
        info.pAttachments = &res.swapchain.getImageViews()[i];
        info.width = res.swapchain.getExtent().width;
        info.height = res.swapchain.getExtent().height;
        info.layers = 1;
        vkCreateFramebuffer(m_device.get(), &info, nullptr, &res.framebuffers[i]);
    }
    
    res.cmdBuffers = m_command.allocateCommandBuffers(2);
    
    uint32_t imageCount = res.swapchain.getImageCount();
    res.imageAvailableSemaphores.resize(2);
    res.renderFinishedSemaphores.resize(imageCount);
    
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (uint32_t i = 0; i < 2; i++) {
        vkCreateSemaphore(m_device.get(), &semInfo, nullptr, &res.imageAvailableSemaphores[i]);
    }
    for (uint32_t i = 0; i < imageCount; i++) {
        vkCreateSemaphore(m_device.get(), &semInfo, nullptr, &res.renderFinishedSemaphores[i]);
    }
    
    return true;
}

void App::cleanupWindowModeResources(WindowModeResources& res) {
    for (auto sem : res.imageAvailableSemaphores) {
        vkDestroySemaphore(m_device.get(), sem, nullptr);
    }
    for (auto sem : res.renderFinishedSemaphores) {
        vkDestroySemaphore(m_device.get(), sem, nullptr);
    }
    for (auto fb : res.framebuffers) {
        vkDestroyFramebuffer(m_device.get(), fb, nullptr);
    }
    m_command.freeCommandBuffers(res.cmdBuffers);
    res.sync.destroy();
    res.renderPass.destroy();
    res.swapchain.destroy();
}

bool App::renderWindowFrame(WindowModeResources& res, DemoBase* demo, size_t frameIndex) {
    res.sync.waitFence(frameIndex);
    
    uint32_t imageIndex = res.swapchain.acquireNextImage(
        res.imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE);
    
    vkResetCommandBuffer(res.cmdBuffers[frameIndex], 0);
    VkCommandBuffer cmd = res.cmdBuffers[frameIndex];
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);
    
    demo->recordCommandBuffer(cmd);
    
    VkImage outputImage = demo->getOutputImage();
    VkExtent2D outputExtent = demo->getOutputExtent();
    VkImage swapchainImage = res.swapchain.getImages()[imageIndex];
    
    VkImageMemoryBarrier dstBarrier{};
    dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    dstBarrier.srcAccessMask = 0;
    dstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    dstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    dstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    dstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dstBarrier.image = swapchainImage;
    dstBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    dstBarrier.subresourceRange.baseMipLevel = 0;
    dstBarrier.subresourceRange.levelCount = 1;
    dstBarrier.subresourceRange.baseArrayLayer = 0;
    dstBarrier.subresourceRange.layerCount = 1;
    
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &dstBarrier);
    
    VkImageBlit blit{};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {static_cast<int32_t>(outputExtent.width), static_cast<int32_t>(outputExtent.height), 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {static_cast<int32_t>(res.swapchain.getExtent().width), static_cast<int32_t>(res.swapchain.getExtent().height), 1};
    
    vkCmdBlitImage(cmd, outputImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
    
    VkImageMemoryBarrier presentBarrier{};
    presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    presentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    presentBarrier.dstAccessMask = 0;
    presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    presentBarrier.image = swapchainImage;
    presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    presentBarrier.subresourceRange.baseMipLevel = 0;
    presentBarrier.subresourceRange.levelCount = 1;
    presentBarrier.subresourceRange.baseArrayLayer = 0;
    presentBarrier.subresourceRange.layerCount = 1;
    
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentBarrier);
    
    vkEndCommandBuffer(cmd);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSems[] = {res.imageAvailableSemaphores[frameIndex]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSems;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    VkSemaphore signalSems[] = {res.renderFinishedSemaphores[imageIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSems;
    
    VkFence fence = res.sync.getInFlightFence(frameIndex);
    vkResetFences(m_device.get(), 1, &fence);
    vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, fence);
    res.swapchain.present(m_device.getPresentQueue(), imageIndex, res.renderFinishedSemaphores[imageIndex]);
    
    return true;
}
#endif

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
#ifdef __ANDROID__
    bool isWindowMode = false;
#else
    bool isWindowMode = !args.offscreen;
#endif
    uint32_t maxFrames = args.frameCount;
    
    std::string outputDir = setupOutputDirectory(demo->getName(), args, isWindowMode);
    
    DemoContext context = createDemoContext(isWindowMode);
    
    if (!demo->init(context)) {
        LOGE("Failed to initialize demo.");
        exitCode = 1;
        return outputDir;
    }
    
#ifndef __ANDROID__
    WindowModeResources windowRes;
    if (isWindowMode) {
        initWindowModeResources(windowRes);
    }
#endif
    
    vk_test::TestRunner testRunner(m_goldenPath, outputDir);
    uint32_t passCount = 0;
    uint32_t failCount = 0;
    uint32_t frameCount = 0;
    size_t currentFrame = 0;
    
    while (true) {
#ifndef __ANDROID__
        if (isWindowMode) {
            glfwPollEvents();
            if (glfwWindowShouldClose(m_window)) break;
            if (maxFrames > 0 && frameCount >= maxFrames) break;
            
            size_t frameIndex = currentFrame % 2;
            renderWindowFrame(windowRes, demo.get(), frameIndex);
            currentFrame++;
        } else
#endif
        {
            uint32_t targetFrames = maxFrames > 0 ? maxFrames : 1;
            if (frameCount >= targetFrames) break;
            
            renderOffscreenFrame(demo.get(), outputDir, frameCount, testRunner, passCount, failCount);
        }
        
        frameCount++;
        demo->update(0.016f);
    }
    
    m_device.waitIdle();
    
#ifndef __ANDROID__
    if (isWindowMode) {
        cleanupWindowModeResources(windowRes);
    }
#endif
    
    if (!isWindowMode) {
        if (m_testMode) {
            printTestSummary(passCount, failCount, frameCount, true);
            exitCode = (failCount > 0) ? 1 : 0;
        } else if (!m_updateGolden) {
            LOGI("Done. %u frames saved to %s", frameCount, outputDir.c_str());
        }
    }
    
    return outputDir;
}

int App::runAllDemosTests(const vk_utils::Args& args) {
#ifndef __ANDROID__
    if (!initWindow(args.width, args.height)) {
        return 1;
    }
#endif
    
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
    
    makeDirectory(args.outputPath);
    makeDirectory(args.goldenPath);
    
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
        makeDirectory(outputDir);
        
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

} // namespace vk_demo