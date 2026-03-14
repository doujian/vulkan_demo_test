#include "render_target.h"
#include "device.h"
#include <cstring>

namespace vk_core {

RenderTarget::~RenderTarget() {
    destroy();
}

RenderTarget::RenderTarget(RenderTarget&& other) noexcept
    : m_device(other.m_device)
    , m_physicalDevice(other.m_physicalDevice)
    , m_colorImage(std::move(other.m_colorImage))
    , m_depthImage(std::move(other.m_depthImage))
    , m_hasDepth(other.m_hasDepth)
    , m_renderPass(other.m_renderPass)
    , m_framebuffer(other.m_framebuffer)
    , m_currentLayout(other.m_currentLayout)
    , m_initialized(other.m_initialized) {
    other.m_device = VK_NULL_HANDLE;
    other.m_renderPass = VK_NULL_HANDLE;
    other.m_framebuffer = VK_NULL_HANDLE;
    other.m_initialized = false;
}

RenderTarget& RenderTarget::operator=(RenderTarget&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_physicalDevice = other.m_physicalDevice;
        m_colorImage = std::move(other.m_colorImage);
        m_depthImage = std::move(other.m_depthImage);
        m_hasDepth = other.m_hasDepth;
        m_renderPass = other.m_renderPass;
        m_framebuffer = other.m_framebuffer;
        m_currentLayout = other.m_currentLayout;
        m_initialized = other.m_initialized;
        
        other.m_device = VK_NULL_HANDLE;
        other.m_renderPass = VK_NULL_HANDLE;
        other.m_framebuffer = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

bool RenderTarget::init(VkDevice device, VkPhysicalDevice physicalDevice,
                        uint32_t width, uint32_t height, VkFormat colorFormat,
                        VkImageUsageFlags additionalUsage, bool enableDepth) {
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_hasDepth = enableDepth;
    
    ImageCreateInfo imageInfo{};
    imageInfo.device = device;
    imageInfo.physicalDevice = physicalDevice;
    imageInfo.width = width;
    imageInfo.height = height;
    imageInfo.format = colorFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | additionalUsage;
    imageInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    if (!m_colorImage.init(imageInfo)) {
        return false;
    }
    
    if (!m_colorImage.createImageView(VK_IMAGE_ASPECT_COLOR_BIT)) {
        return false;
    }
    
    if (enableDepth) {
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
        
        ImageCreateInfo depthInfo{};
        depthInfo.device = device;
        depthInfo.physicalDevice = physicalDevice;
        depthInfo.width = width;
        depthInfo.height = height;
        depthInfo.format = depthFormat;
        depthInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        depthInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depthInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        
        if (!m_depthImage.init(depthInfo)) {
            return false;
        }
        
        if (!m_depthImage.createImageView(VK_IMAGE_ASPECT_DEPTH_BIT)) {
            return false;
        }
    }
    
    std::vector<VkAttachmentDescription> attachments;
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = colorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments.push_back(colorAttachment);
    
    VkAttachmentDescription depthAttachment{};
    if (enableDepth) {
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(depthAttachment);
    }
    
    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    if (enableDepth) {
        subpass.pDepthStencilAttachment = &depthRef;
    }
    
    std::vector<VkSubpassDependency> dependencies;
    
    if (enableDepth) {
        VkSubpassDependency depthDependency{};
        depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        depthDependency.dstSubpass = 0;
        depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depthDependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        dependencies.push_back(depthDependency);
    }

    VkSubpassDependency colorDependencyIn{};
    colorDependencyIn.srcSubpass = VK_SUBPASS_EXTERNAL;
    colorDependencyIn.dstSubpass = 0;
    colorDependencyIn.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    colorDependencyIn.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorDependencyIn.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    colorDependencyIn.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    colorDependencyIn.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies.push_back(colorDependencyIn);

    VkSubpassDependency colorDependencyOut{};
    colorDependencyOut.srcSubpass = 0;
    colorDependencyOut.dstSubpass = VK_SUBPASS_EXTERNAL;
    colorDependencyOut.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorDependencyOut.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    colorDependencyOut.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    colorDependencyOut.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    colorDependencyOut.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies.push_back(colorDependencyOut);
    
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();
    
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        return false;
    }
    
    std::vector<VkImageView> fbAttachments;
    fbAttachments.push_back(m_colorImage.getView());
    if (enableDepth) {
        fbAttachments.push_back(m_depthImage.getView());
    }
    
    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = m_renderPass;
    fbInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
    fbInfo.pAttachments = fbAttachments.data();
    fbInfo.width = width;
    fbInfo.height = height;
    fbInfo.layers = 1;
    
    if (vkCreateFramebuffer(device, &fbInfo, nullptr, &m_framebuffer) != VK_SUCCESS) {
        vkDestroyRenderPass(device, m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
        return false;
    }
    
    m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_initialized = true;
    return true;
}

void RenderTarget::destroy() {
    if (!m_initialized) return;
    
    if (m_framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
        m_framebuffer = VK_NULL_HANDLE;
    }
    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }
    
    m_colorImage.destroy();
    m_depthImage.destroy();
    
    m_initialized = false;
}

void RenderTarget::transitionToColorAttachment(VkCommandBuffer cmd) {
    if (m_currentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) return;
    if (m_currentLayout == VK_IMAGE_LAYOUT_UNDEFINED) return;
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_currentLayout;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_colorImage.get();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    VkPipelineStageFlags srcStage = 0;
    VkAccessFlags srcAccess = 0;
    
    if (m_currentLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        srcAccess = VK_ACCESS_SHADER_READ_BIT;
    } else if (m_currentLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        srcAccess = VK_ACCESS_TRANSFER_READ_BIT;
    }
    
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    m_currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

void RenderTarget::transitionToShaderRead(VkCommandBuffer cmd) {
    if (m_currentLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) return;
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_currentLayout;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_colorImage.get();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    m_currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void RenderTarget::transitionToTransferSrc(VkCommandBuffer cmd) {
    if (m_currentLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) return;
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_currentLayout;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_colorImage.get();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkAccessFlags srcAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    
    vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    m_currentLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
}

void RenderTarget::beginRenderPass(VkCommandBuffer cmd, const float clearColor[4], float clearDepth) {
    float defaultClear[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    const float* clear = clearColor ? clearColor : defaultClear;
    
    VkRenderPassBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = m_renderPass;
    beginInfo.framebuffer = m_framebuffer;
    beginInfo.renderArea.offset = {0, 0};
    beginInfo.renderArea.extent = {m_colorImage.getWidth(), m_colorImage.getHeight()};
    
    VkClearValue clearValues[2];
    clearValues[0].color.float32[0] = clear[0];
    clearValues[0].color.float32[1] = clear[1];
    clearValues[0].color.float32[2] = clear[2];
    clearValues[0].color.float32[3] = clear[3];
    clearValues[1].depthStencil.depth = clearDepth;
    clearValues[1].depthStencil.stencil = 0;
    
    beginInfo.clearValueCount = m_hasDepth ? 2 : 1;
    beginInfo.pClearValues = clearValues;
    
    vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderTarget::endRenderPass(VkCommandBuffer cmd) {
    vkCmdEndRenderPass(cmd);
    m_currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

} // namespace vk_core