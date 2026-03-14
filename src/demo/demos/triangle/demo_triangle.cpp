#include "demo/demo_base.h"
#include "demo/demo_registry.h"
#include "core/pipeline.h"
#include "core/descriptor.h"
#include "core/render_target.h"
#include <vector>
#include <array>
#include <memory>

namespace vk_demo {

class DemoTriangle : public DemoBase {
public:
    std::string getName() const override { return "triangle"; }
    std::string getDescription() const override { return "Renders a simple colored triangle"; }
    bool needDepth() const override { return false; }
    
    bool init(const DemoContext& context) override {
        m_context = context;
        
        if (!m_renderTarget.init(context.device, context.physicalDevice,
                                  context.extent.width, context.extent.height,
                                  context.colorFormat,
                                  VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {
            return false;
        }
        
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        
        if (vkCreatePipelineLayout(context.device, &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
            return false;
        }
        
        vk_core::PipelineConfig config = vk_core::Pipeline::getDefaultConfig(m_renderTarget.getRenderPass(), m_renderTarget.getExtent());
        config.pipelineLayout = m_pipelineLayout;
        vk_core::Pipeline::enableDepthTest(config, false);
        
        m_pipeline = std::make_unique<vk_core::Pipeline>();
        return m_pipeline->init(context.device, config, "shaders/triangle/triangle.vert.spv", "shaders/triangle/triangle.frag.spv");
    }
    
    void destroy() override {
        if (m_pipeline) {
            m_pipeline->destroy();
            m_pipeline.reset();
        }
        if (m_pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_context.device, m_pipelineLayout, nullptr);
            m_pipelineLayout = VK_NULL_HANDLE;
        }
        m_renderTarget.destroy();
    }
    
    void recordCommandBuffer(VkCommandBuffer cmd) override {
        float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        m_renderTarget.beginRenderPass(cmd, clearColor);
        
        VkViewport viewport{};
        viewport.width = static_cast<float>(m_renderTarget.getExtent().width);
        viewport.height = static_cast<float>(m_renderTarget.getExtent().height);
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        
        VkRect2D scissor{};
        scissor.extent = m_renderTarget.getExtent();
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        
        m_pipeline->bind(cmd);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        
        m_renderTarget.endRenderPass(cmd);
        m_renderTarget.transitionToTransferSrc(cmd);
    }
    
    VkImage getOutputImage() const override { return m_renderTarget.getColorImage(); }
    VkExtent2D getOutputExtent() const override { return m_renderTarget.getExtent(); }

private:
    DemoContext m_context;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    std::unique_ptr<vk_core::Pipeline> m_pipeline;
    vk_core::RenderTarget m_renderTarget;
};

REGISTER_DEMO(DemoTriangle, "triangle", "Renders a simple colored triangle")

} // namespace vk_demo