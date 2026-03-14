#include "pipeline.h"
#include "shader.h"
#include <iostream>

namespace vk_core {

bool Pipeline::init(VkDevice device, const PipelineConfig& config,
                    const std::string& vertShaderPath, const std::string& fragShaderPath) {
    if (m_initialized) return true;
    
    m_device = device;
    
    auto vertShaderCode = Shader::readFile(vertShaderPath);
    auto fragShaderCode = Shader::readFile(fragShaderPath);
    
    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        std::cerr << "Failed to read shader files!" << std::endl;
        return false;
    }
    
    VkShaderModule vertShaderModule = Shader::createModule(device, vertShaderCode);
    VkShaderModule fragShaderModule = Shader::createModule(device, fragShaderCode);
    
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        return false;
    }
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &config.vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &config.inputAssembly;
    pipelineInfo.pViewportState = nullptr;
    pipelineInfo.pRasterizationState = &config.rasterizer;
    pipelineInfo.pMultisampleState = &config.multisampling;
    pipelineInfo.pDepthStencilState = &config.depthStencil;
    pipelineInfo.pColorBlendState = &config.colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = config.pipelineLayout;
    pipelineInfo.renderPass = config.renderPass;
    pipelineInfo.subpass = config.subpass;
    
    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    pipelineInfo.pDynamicState = &dynamicState;
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    pipelineInfo.pViewportState = &viewportState;
    
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        std::cerr << "Failed to create graphics pipeline!" << std::endl;
        return false;
    }
    
    m_layout = config.pipelineLayout;
    
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    
    m_initialized = true;
    return true;
}

void Pipeline::bind(VkCommandBuffer cmd) const {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

PipelineConfig Pipeline::getDefaultConfig(VkRenderPass renderPass, VkExtent2D extent) {
    PipelineConfig config{};
    
    config.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    config.inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    config.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    config.inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    config.viewport.x = 0.0f;
    config.viewport.y = 0.0f;
    config.viewport.width = static_cast<float>(extent.width);
    config.viewport.height = static_cast<float>(extent.height);
    config.viewport.minDepth = 0.0f;
    config.viewport.maxDepth = 1.0f;
    
    config.scissor.offset = {0, 0};
    config.scissor.extent = extent;
    
    config.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    config.rasterizer.depthClampEnable = VK_FALSE;
    config.rasterizer.rasterizerDiscardEnable = VK_FALSE;
    config.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    config.rasterizer.lineWidth = 1.0f;
    config.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    config.rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    config.rasterizer.depthBiasEnable = VK_FALSE;
    
    config.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    config.multisampling.sampleShadingEnable = VK_FALSE;
    config.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    config.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    config.colorBlendAttachment.blendEnable = VK_FALSE;
    
    config.colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    config.colorBlending.logicOpEnable = VK_FALSE;
    config.colorBlending.attachmentCount = 1;
    config.colorBlending.pAttachments = &config.colorBlendAttachment;
    
    config.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    config.depthStencil.depthTestEnable = VK_TRUE;
    config.depthStencil.depthWriteEnable = VK_TRUE;
    config.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    config.depthStencil.depthBoundsTestEnable = VK_FALSE;
    config.depthStencil.stencilTestEnable = VK_FALSE;
    
    config.renderPass = renderPass;
    config.extent = extent;
    
    return config;
}

void Pipeline::enableDepthTest(PipelineConfig& config, bool enable) {
    config.depthStencil.depthTestEnable = enable ? VK_TRUE : VK_FALSE;
    config.depthStencil.depthWriteEnable = enable ? VK_TRUE : VK_FALSE;
}

void Pipeline::setWireframe(PipelineConfig& config, bool wireframe) {
    config.rasterizer.polygonMode = wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
}

} // namespace vk_core