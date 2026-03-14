#include "demo/demo_base.h"
#include "demo/demo_registry.h"
#include "core/pipeline.h"
#include "core/compute_pipeline.h"
#include "core/descriptor.h"
#include "core/sampler.h"
#include "core/render_target.h"
#include <vector>
#include <memory>

namespace vk_demo {

struct PushConstants {
    uint32_t width;
    uint32_t height;
};

class DemoMultiPassBlend : public DemoBase {
public:
    std::string getName() const override { return "multi_pass_blend"; }
    std::string getDescription() const override { return "Multi-pass rendering: triangle + compute gradient + blend"; }
    bool needDepth() const override { return false; }
    
    bool init(const DemoContext& context) override {
        m_context = context;
        
        if (!initRenderTargets()) return false;
        if (!initTrianglePipeline()) return false;
        if (!initComputePipeline()) return false;
        if (!initBlendPipeline()) return false;
        
        return true;
    }
    
    void destroy() override {
        m_blendPipeline.reset();
        m_computePipeline.reset();
        m_trianglePipeline.reset();
        
        if (m_blendDescriptorSet != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(m_context.device, m_descriptorPool, 1, &m_blendDescriptorSet);
        }
        if (m_computeDescriptorSet != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(m_context.device, m_descriptorPool, 1, &m_computeDescriptorSet);
        }
        if (m_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_context.device, m_descriptorPool, nullptr);
        }
        m_blendSetLayout.destroy();
        m_computeSetLayout.destroy();
        if (m_blendLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_context.device, m_blendLayout, nullptr);
        }
        if (m_computeLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_context.device, m_computeLayout, nullptr);
        }
        if (m_triangleLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_context.device, m_triangleLayout, nullptr);
        }
        
        m_sampler.destroy();
        m_gradientTexture.destroy();
        m_triangleRT.destroy();
        m_outputRT.destroy();
    }
    
    void recordCommandBuffer(VkCommandBuffer cmd) override {
        renderTrianglePass(cmd);
        renderComputePass(cmd);
        renderBlendPass(cmd);
    }
    
    VkImage getOutputImage() const override { return m_outputRT.getColorImage(); }
    VkExtent2D getOutputExtent() const override { return m_outputRT.getExtent(); }

private:
    bool initRenderTargets() {
        if (!m_triangleRT.init(m_context.device, m_context.physicalDevice,
                               m_context.extent.width, m_context.extent.height,
                               VK_FORMAT_R8G8B8A8_SRGB)) {
            return false;
        }
        
        if (!m_gradientTexture.init(m_context.device, m_context.physicalDevice,
                                    m_context.extent.width, m_context.extent.height,
                                    VK_FORMAT_R8G8B8A8_UNORM,
                                    VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)) {
            return false;
        }
        
        if (!m_outputRT.init(m_context.device, m_context.physicalDevice,
                             m_context.extent.width, m_context.extent.height,
                             m_context.colorFormat,
                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {
            return false;
        }
        
        if (!m_sampler.init(m_context.device)) {
            return false;
        }
        
        return true;
    }
    
    bool initTrianglePipeline() {
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        
        if (vkCreatePipelineLayout(m_context.device, &layoutInfo, nullptr, &m_triangleLayout) != VK_SUCCESS) {
            return false;
        }
        
        m_trianglePipeline = std::make_unique<vk_core::Pipeline>();
        vk_core::PipelineConfig config = vk_core::Pipeline::getDefaultConfig(m_triangleRT.getRenderPass(), m_triangleRT.getExtent());
        config.pipelineLayout = m_triangleLayout;
        vk_core::Pipeline::enableDepthTest(config, false);
        
        return m_trianglePipeline->init(m_context.device, config, 
                                        "shaders/multi_pass_blend/blue_triangle.vert.spv", 
                                        "shaders/multi_pass_blend/blue_triangle.frag.spv");
    }
    
    bool initComputePipeline() {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back(vk_core::Descriptor::createStorageImageBinding(0, VK_SHADER_STAGE_COMPUTE_BIT));
        
        if (!m_computeSetLayout.init(m_context.device, bindings)) {
            return false;
        }
        
        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushRange.offset = 0;
        pushRange.size = sizeof(PushConstants);
        
        VkDescriptorSetLayout setLayout = m_computeSetLayout.get();
        
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &setLayout;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;
        
        if (vkCreatePipelineLayout(m_context.device, &layoutInfo, nullptr, &m_computeLayout) != VK_SUCCESS) {
            return false;
        }
        
        m_computePipeline = std::make_unique<vk_core::ComputePipeline>();
        if (!m_computePipeline->init(m_context.device, m_computeLayout, "shaders/multi_pass_blend/gradient.comp.spv")) {
            return false;
        }
        
        std::vector<VkDescriptorPoolSize> poolSizes;
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1});
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2});
        
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 2;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        
        if (vkCreateDescriptorPool(m_context.device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
            return false;
        }
        
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &setLayout;
        
        if (vkAllocateDescriptorSets(m_context.device, &allocInfo, &m_computeDescriptorSet) != VK_SUCCESS) {
            return false;
        }
        
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = m_gradientTexture.getColorImageView();
        
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_computeDescriptorSet;
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;
        
        vkUpdateDescriptorSets(m_context.device, 1, &write, 0, nullptr);
        
        return true;
    }
    
    bool initBlendPipeline() {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back(vk_core::Descriptor::createSamplerBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT));
        bindings.push_back(vk_core::Descriptor::createSamplerBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT));
        
        if (!m_blendSetLayout.init(m_context.device, bindings)) {
            return false;
        }
        
        VkDescriptorSetLayout setLayout = m_blendSetLayout.get();
        
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &setLayout;
        
        if (vkCreatePipelineLayout(m_context.device, &layoutInfo, nullptr, &m_blendLayout) != VK_SUCCESS) {
            return false;
        }
        
        m_blendPipeline = std::make_unique<vk_core::Pipeline>();
        vk_core::PipelineConfig config = vk_core::Pipeline::getDefaultConfig(m_outputRT.getRenderPass(), m_outputRT.getExtent());
        config.pipelineLayout = m_blendLayout;
        vk_core::Pipeline::enableDepthTest(config, false);
        
        if (!m_blendPipeline->init(m_context.device, config,
                                   "shaders/multi_pass_blend/blend.vert.spv",
                                   "shaders/multi_pass_blend/blend.frag.spv")) {
            return false;
        }
        
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &setLayout;
        
        if (vkAllocateDescriptorSets(m_context.device, &allocInfo, &m_blendDescriptorSet) != VK_SUCCESS) {
            return false;
        }
        
        VkDescriptorImageInfo tex1Info{};
        tex1Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        tex1Info.imageView = m_triangleRT.getColorImageView();
        tex1Info.sampler = m_sampler.get();
        
        VkDescriptorImageInfo tex2Info{};
        tex2Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        tex2Info.imageView = m_gradientTexture.getColorImageView();
        tex2Info.sampler = m_sampler.get();
        
        VkWriteDescriptorSet writes[2] = {};
        
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_blendDescriptorSet;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &tex1Info;
        
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_blendDescriptorSet;
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &tex2Info;
        
        vkUpdateDescriptorSets(m_context.device, 2, writes, 0, nullptr);
        
        return true;
    }
    
    void renderTrianglePass(VkCommandBuffer cmd) {
        m_triangleRT.transitionToColorAttachment(cmd);
        
        float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        m_triangleRT.beginRenderPass(cmd, clearColor);
        
        VkViewport viewport{};
        viewport.width = static_cast<float>(m_triangleRT.getExtent().width);
        viewport.height = static_cast<float>(m_triangleRT.getExtent().height);
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        
        VkRect2D scissor{};
        scissor.extent = m_triangleRT.getExtent();
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        
        m_trianglePipeline->bind(cmd);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        
        m_triangleRT.endRenderPass(cmd);
    }
    
    void renderComputePass(VkCommandBuffer cmd) {
        VkImage gradientImage = m_gradientTexture.getColorImage();
        transitionImageToGeneral(cmd, gradientImage);
        
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline->get());
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computeLayout,
                                0, 1, &m_computeDescriptorSet, 0, nullptr);
        
        PushConstants pushConstants{};
        pushConstants.width = m_context.extent.width;
        pushConstants.height = m_context.extent.height;
        
        vkCmdPushConstants(cmd, m_computeLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                           0, sizeof(PushConstants), &pushConstants);
        
        uint32_t groupX = (m_context.extent.width + 7) / 8;
        uint32_t groupY = (m_context.extent.height + 7) / 8;
        
        m_computePipeline->dispatch(cmd, groupX, groupY);
        
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = gradientImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
    
    void renderBlendPass(VkCommandBuffer cmd) {
        m_triangleRT.transitionToShaderRead(cmd);
        m_outputRT.transitionToColorAttachment(cmd);
        
        m_outputRT.beginRenderPass(cmd, nullptr);
        
        VkViewport viewport{};
        viewport.width = static_cast<float>(m_outputRT.getExtent().width);
        viewport.height = static_cast<float>(m_outputRT.getExtent().height);
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        
        VkRect2D scissor{};
        scissor.extent = m_outputRT.getExtent();
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        
        m_blendPipeline->bind(cmd);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_blendLayout,
                                0, 1, &m_blendDescriptorSet, 0, nullptr);
        vkCmdDraw(cmd, 6, 1, 0, 0);
        
        m_outputRT.endRenderPass(cmd);
        m_outputRT.transitionToTransferSrc(cmd);
    }
    
    void transitionImageToGeneral(VkCommandBuffer cmd, VkImage image) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

private:
    DemoContext m_context;
    
    vk_core::RenderTarget m_triangleRT;
    vk_core::RenderTarget m_gradientTexture;
    vk_core::RenderTarget m_outputRT;
    vk_core::Sampler m_sampler;
    
    VkPipelineLayout m_triangleLayout = VK_NULL_HANDLE;
    std::unique_ptr<vk_core::Pipeline> m_trianglePipeline;
    
    VkPipelineLayout m_computeLayout = VK_NULL_HANDLE;
    vk_core::DescriptorSetLayout m_computeSetLayout;
    std::unique_ptr<vk_core::ComputePipeline> m_computePipeline;
    VkDescriptorSet m_computeDescriptorSet = VK_NULL_HANDLE;
    
    VkPipelineLayout m_blendLayout = VK_NULL_HANDLE;
    vk_core::DescriptorSetLayout m_blendSetLayout;
    std::unique_ptr<vk_core::Pipeline> m_blendPipeline;
    VkDescriptorSet m_blendDescriptorSet = VK_NULL_HANDLE;
    
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
};

REGISTER_DEMO(DemoMultiPassBlend, "multi_pass_blend", "Multi-pass rendering: triangle + compute gradient + blend")

} // namespace vk_demo