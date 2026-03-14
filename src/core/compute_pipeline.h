#pragma once

#include "pipeline_base.h"
#include <string>

namespace vk_core {

class ComputePipeline : public PipelineBase {
public:
    ComputePipeline() = default;
    
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;
    ComputePipeline(ComputePipeline&&) noexcept = default;
    ComputePipeline& operator=(ComputePipeline&&) noexcept = default;
    
    bool init(VkDevice device, VkPipelineLayout layout, const std::string& shaderPath);
    
    void bind(VkCommandBuffer cmd) const override;
    void dispatch(VkCommandBuffer cmd, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ = 1) const;
};

} // namespace vk_core