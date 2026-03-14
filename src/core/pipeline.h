#pragma once

#include "pipeline_base.h"
#include <vector>
#include <string>

namespace vk_core {

struct PipelineConfig {
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    VkViewport viewport{};
    VkRect2D scissor{};
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineMultisampleStateCreateInfo multisampling{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t subpass = 0;
    VkExtent2D extent = {0, 0};
};

class Pipeline : public PipelineBase {
public:
    Pipeline() = default;
    
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&) noexcept = default;
    Pipeline& operator=(Pipeline&&) noexcept = default;
    
    bool init(VkDevice device, const PipelineConfig& config,
              const std::string& vertShaderPath, const std::string& fragShaderPath);
    
    void bind(VkCommandBuffer cmd) const override;
    
    static PipelineConfig getDefaultConfig(VkRenderPass renderPass, VkExtent2D extent);
    static void enableDepthTest(PipelineConfig& config, bool enable = true);
    static void setWireframe(PipelineConfig& config, bool wireframe = true);
};

} // namespace vk_core