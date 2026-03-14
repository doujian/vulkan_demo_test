#include "compute_pipeline.h"
#include "shader.h"

namespace vk_core {

bool ComputePipeline::init(VkDevice device, VkPipelineLayout layout, const std::string& shaderPath) {
    m_device = device;
    m_layout = layout;
    
    auto code = Shader::readFile(shaderPath);
    if (code.empty()) {
        return false;
    }
    
    VkShaderModule shaderModule = Shader::createModule(device, code);
    if (shaderModule == VK_NULL_HANDLE) {
        return false;
    }
    
    VkPipelineShaderStageCreateInfo shaderStage{};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStage.module = shaderModule;
    shaderStage.pName = "main";
    
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = layout;
    pipelineInfo.stage = shaderStage;
    
    VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    
    vkDestroyShaderModule(device, shaderModule, nullptr);
    
    if (result != VK_SUCCESS) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void ComputePipeline::bind(VkCommandBuffer cmd) const {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
}

void ComputePipeline::dispatch(VkCommandBuffer cmd, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const {
    vkCmdDispatch(cmd, groupCountX, groupCountY, groupCountZ);
}

} // namespace vk_core