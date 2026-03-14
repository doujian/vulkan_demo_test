#include "demo/demo_registry.h"
#include "demo/demo_base.h"
#include "core/instance.h"
#include "core/device.h"
#include "core/command.h"
#include "core/swapchain.h"
#include "core/render_pass.h"
#include "core/frame_buffer.h"
#include "core/sync.h"

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include <cstring>
#include <chrono>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "VulkanDemo", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "VulkanDemo", __VA_ARGS__)

using namespace vk_core;
using namespace vk_demo;

namespace {

struct AndroidAppContext {
    Instance instance;
    Device device;
    Command command;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    Swapchain swapchain;
    RenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;
    Sync sync;
    std::vector<VkCommandBuffer> cmdBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::unique_ptr<DemoBase> demo;
    DemoContext demoContext;
    ANativeWindow* window = nullptr;
    bool initialized = false;
    size_t currentFrame = 0;
    std::string demoName;
    float lastTime = 0.0f;
};

static AndroidAppContext g_app;

bool initVulkan(ANativeWindow* window, const std::string& demoName) {
    if (g_app.initialized) {
        return true;
    }
    
    g_app.window = window;
    g_app.demoName = demoName;
    
    if (!g_app.instance.init({}, {}, false)) {
        LOGE("Failed to create Vulkan instance");
        return false;
    }
    
    VkAndroidSurfaceCreateInfoKHR surfaceInfo{};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.window = window;
    
    if (vkCreateAndroidSurfaceKHR(g_app.instance.get(), &surfaceInfo, nullptr, &g_app.surface) != VK_SUCCESS) {
        LOGE("Failed to create Android surface");
        return false;
    }
    
    std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    if (!g_app.device.init(g_app.instance.get(), g_app.surface, extensions)) {
        LOGE("Failed to create device");
        return false;
    }
    
    if (!g_app.command.init(g_app.device.get(), g_app.device.getGraphicsQueueFamily())) {
        LOGE("Failed to create command pool");
        return false;
    }
    
    int32_t width = ANativeWindow_getWidth(window);
    int32_t height = ANativeWindow_getHeight(window);
    LOGI("Window size: %dx%d", width, height);
    
    g_app.swapchain.init(g_app.device.get(), g_app.device.getPhysicalDevice(), 
                          g_app.surface, width, height, true);
    
    g_app.renderPass.init(g_app.device.get(), g_app.swapchain.getImageFormat());
    
    g_app.sync.init(g_app.device.get(), 2);
    
    g_app.framebuffers.resize(g_app.swapchain.getImageCount());
    for (size_t i = 0; i < g_app.swapchain.getImageCount(); i++) {
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = g_app.renderPass.get();
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &g_app.swapchain.getImageViews()[i];
        fbInfo.width = g_app.swapchain.getExtent().width;
        fbInfo.height = g_app.swapchain.getExtent().height;
        fbInfo.layers = 1;
        vkCreateFramebuffer(g_app.device.get(), &fbInfo, nullptr, &g_app.framebuffers[i]);
    }
    
    g_app.cmdBuffers = g_app.command.allocateCommandBuffers(2);
    
    uint32_t imageCount = g_app.swapchain.getImageCount();
    g_app.imageAvailableSemaphores.resize(2);
    g_app.renderFinishedSemaphores.resize(imageCount);
    
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (uint32_t i = 0; i < 2; i++) {
        vkCreateSemaphore(g_app.device.get(), &semInfo, nullptr, &g_app.imageAvailableSemaphores[i]);
    }
    for (uint32_t i = 0; i < imageCount; i++) {
        vkCreateSemaphore(g_app.device.get(), &semInfo, nullptr, &g_app.renderFinishedSemaphores[i]);
    }
    
    forceLinkDemos();
    
    g_app.demo = DemoRegistry::instance().createDemo(demoName);
    if (!g_app.demo) {
        LOGE("Demo '%s' not found", demoName.c_str());
        return false;
    }
    
    g_app.demoContext.instance = g_app.instance.get();
    g_app.demoContext.physicalDevice = g_app.device.getPhysicalDevice();
    g_app.demoContext.device = g_app.device.get();
    g_app.demoContext.graphicsQueue = g_app.device.getGraphicsQueue();
    g_app.demoContext.presentQueue = g_app.device.getPresentQueue();
    g_app.demoContext.commandPool = g_app.command.getPool();
    g_app.demoContext.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    g_app.demoContext.colorFormat = g_app.swapchain.getImageFormat();
    g_app.demoContext.graphicsQueueFamily = g_app.device.getGraphicsQueueFamily();
    g_app.demoContext.presentQueueFamily = g_app.device.getPresentQueueFamily();
    
    if (!g_app.demo->init(g_app.demoContext)) {
        LOGE("Failed to initialize demo");
        return false;
    }
    
    g_app.initialized = true;
    g_app.lastTime = 0.0f;
    LOGI("Vulkan initialized successfully for demo: %s", demoName.c_str());
    return true;
}

void renderFrame() {
    if (!g_app.initialized) return;
    
    size_t frameIndex = g_app.currentFrame % 2;
    
    g_app.sync.waitFence(frameIndex);
    
    uint32_t imageIndex = g_app.swapchain.acquireNextImage(
        g_app.imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE);
    
    vkResetCommandBuffer(g_app.cmdBuffers[frameIndex], 0);
    VkCommandBuffer cmd = g_app.cmdBuffers[frameIndex];
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);
    
    g_app.demo->recordCommandBuffer(cmd);
    
    VkImage outputImage = g_app.demo->getOutputImage();
    VkExtent2D outputExtent = g_app.demo->getOutputExtent();
    VkImage swapchainImage = g_app.swapchain.getImages()[imageIndex];
    
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
    blit.dstOffsets[1] = {static_cast<int32_t>(g_app.swapchain.getExtent().width), 
                           static_cast<int32_t>(g_app.swapchain.getExtent().height), 1};
    
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
    VkSemaphore waitSems[] = {g_app.imageAvailableSemaphores[frameIndex]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSems;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    VkSemaphore signalSems[] = {g_app.renderFinishedSemaphores[imageIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSems;
    
    VkFence fence = g_app.sync.getInFlightFence(frameIndex);
    vkResetFences(g_app.device.get(), 1, &fence);
    vkQueueSubmit(g_app.device.getGraphicsQueue(), 1, &submitInfo, fence);
    
    g_app.swapchain.present(g_app.device.getPresentQueue(), imageIndex, 
                            g_app.renderFinishedSemaphores[imageIndex]);
    
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    float currentTime = std::chrono::duration<float>(duration).count();
    float deltaTime = currentTime - g_app.lastTime;
    g_app.lastTime = currentTime;
    
    g_app.demo->update(deltaTime);
    g_app.currentFrame++;
}

void cleanup() {
    if (!g_app.initialized) return;
    
    g_app.device.waitIdle();
    
    if (g_app.demo) {
        g_app.demo->destroy();
        g_app.demo.reset();
    }
    
    for (auto sem : g_app.imageAvailableSemaphores) {
        vkDestroySemaphore(g_app.device.get(), sem, nullptr);
    }
    for (auto sem : g_app.renderFinishedSemaphores) {
        vkDestroySemaphore(g_app.device.get(), sem, nullptr);
    }
    for (auto fb : g_app.framebuffers) {
        vkDestroyFramebuffer(g_app.device.get(), fb, nullptr);
    }
    
    g_app.command.freeCommandBuffers(g_app.cmdBuffers);
    g_app.sync.destroy();
    g_app.renderPass.destroy();
    g_app.swapchain.destroy();
    
    if (g_app.surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(g_app.instance.get(), g_app.surface, nullptr);
        g_app.surface = VK_NULL_HANDLE;
    }
    
    g_app.command.destroy();
    g_app.device.destroy();
    g_app.instance.destroy();
    
    g_app.initialized = false;
    g_app.window = nullptr;
    LOGI("Vulkan cleanup complete");
}

}

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_vulkandemo_VulkanDemo_nativeInit(JNIEnv* env, jobject obj, jobject surface, jstring demoName) {
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        LOGE("Failed to get ANativeWindow from Surface");
        return JNI_FALSE;
    }
    
    const char* demoNameCStr = env->GetStringUTFChars(demoName, nullptr);
    std::string demoNameStr(demoNameCStr);
    env->ReleaseStringUTFChars(demoName, demoNameCStr);
    
    bool result = initVulkan(window, demoNameStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_vulkandemo_VulkanDemo_nativeRender(JNIEnv* env, jobject obj) {
    renderFrame();
}

JNIEXPORT void JNICALL
Java_com_vulkandemo_VulkanDemo_nativeCleanup(JNIEnv* env, jobject obj) {
    cleanup();
}

JNIEXPORT void JNICALL
Java_com_vulkandemo_VulkanDemo_nativeSurfaceDestroyed(JNIEnv* env, jobject obj) {
    cleanup();
}

JNIEXPORT jobjectArray JNICALL
Java_com_vulkandemo_VulkanDemo_nativeGetDemoNames(JNIEnv* env, jobject obj) {
    forceLinkDemos();
    
    auto names = DemoRegistry::instance().getDemoNames();
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray array = env->NewObjectArray(names.size(), stringClass, nullptr);
    
    for (size_t i = 0; i < names.size(); i++) {
        jstring str = env->NewStringUTF(names[i].c_str());
        env->SetObjectArrayElement(array, i, str);
        env->DeleteLocalRef(str);
    }
    
    return array;
}

JNIEXPORT jstring JNICALL
Java_com_vulkandemo_VulkanDemo_nativeGetDemoDescription(JNIEnv* env, jobject obj, jstring demoName) {
    const char* demoNameCStr = env->GetStringUTFChars(demoName, nullptr);
    std::string desc = DemoRegistry::instance().getDescription(demoNameCStr);
    env->ReleaseStringUTFChars(demoName, demoNameCStr);
    return env->NewStringUTF(desc.c_str());
}

}