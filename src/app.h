#pragma once

#include "demo/demo_base.h"
#include "demo/demo_config.h"
#include "utils/args_parser.h"
#include "core/instance.h"
#include "core/device.h"
#include "core/command.h"
#include "core/swapchain.h"
#include "core/render_pass.h"
#include "core/sync.h"
#include "test/test_runner.h"
#include <memory>
#include <string>
#include <vector>

struct GLFWwindow;

namespace vk_demo {

struct WindowModeResources {
    vk_core::Swapchain swapchain;
    vk_core::RenderPass renderPass;
    vk_core::Sync sync;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> cmdBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
};

class App {
public:
    App() = default;
    ~App();
    
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    
    int run(int argc, char* argv[]);

private:
    bool init(const vk_utils::Args& args, std::unique_ptr<DemoBase>& demo);
    void cleanup();
    
    bool initWindow(uint32_t width, uint32_t height);
    bool initVulkan(bool enableValidation);
    
    std::string runDemo(std::unique_ptr<DemoBase>& demo, const vk_utils::Args& args, int& exitCode);
    int runAllDemosTests(const vk_utils::Args& args);
    
    std::string createOutputDirectory(const std::string& demoName, 
                                       const std::string& basePath);
    
    std::string setupOutputDirectory(const std::string& demoName, const vk_utils::Args& args, bool isWindowMode);
    DemoContext createDemoContext(bool isWindowMode);
    bool initWindowModeResources(WindowModeResources& res);
    void cleanupWindowModeResources(WindowModeResources& res);
    bool renderWindowFrame(WindowModeResources& res, DemoBase* demo, size_t frameIndex);
    bool renderOffscreenFrame(DemoBase* demo, const std::string& outputDir, uint32_t frameCount,
                              vk_test::TestRunner& testRunner, uint32_t& passCount, uint32_t& failCount);
    void printTestSummary(uint32_t passCount, uint32_t failCount, uint32_t frameCount, bool isTestMode);
    void makeDirectory(const std::string& path);
    
private:
    GLFWwindow* m_window = nullptr;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    
    vk_core::Instance m_instance;
    vk_core::Device m_device;
    vk_core::Command m_command;
    
    bool m_initialized = false;
    bool m_validationEnabled = false;
    bool m_testMode = false;
    bool m_updateGolden = false;
    std::string m_goldenPath = "golden";
    float m_testThreshold = 0.99f;
};

} // namespace vk_demo