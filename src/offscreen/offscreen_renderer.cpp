#include "offscreen_renderer.h"
#include "../core/device.h"
#include "../utils/png_writer.h"
#include <iostream>
#include <vector>

namespace vk_offscreen {

OffscreenRenderer::~OffscreenRenderer() {
    destroy();
}

bool OffscreenRenderer::init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device,
                              uint32_t width, uint32_t height, VkFormat colorFormat,
                              bool enableDepth, VkFormat depthFormat) {
    if (m_initialized) return true;
    
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_width = width;
    m_height = height;
    
    vk_core::ImageCreateInfo imageInfo{};
    imageInfo.device = device;
    imageInfo.physicalDevice = physicalDevice;
    imageInfo.width = width;
    imageInfo.height = height;
    imageInfo.format = colorFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    if (!m_colorImage.init(imageInfo)) {
        return false;
    }
    
    if (!m_colorImage.createImageView(VK_IMAGE_ASPECT_COLOR_BIT)) {
        return false;
    }
    
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorRefs;
    
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = colorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments.push_back(colorAttachment);
    
    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorRefs.push_back(colorRef);
    
    VkAttachmentReference depthRef{};
    if (enableDepth && depthFormat != VK_FORMAT_UNDEFINED) {
        if (!m_depthImage.init(device, physicalDevice, width, height)) {
            return false;
        }
        
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = vk_core::Image::findDepthFormat(physicalDevice);
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(depthAttachment);
        
        depthRef.attachment = 1;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
    subpass.pColorAttachments = colorRefs.data();
    if (enableDepth && m_depthImage.get() != VK_NULL_HANDLE) {
        subpass.pDepthStencilAttachment = &depthRef;
    }
    
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        return false;
    }
    
    std::vector<VkImageView> fbAttachments;
    fbAttachments.push_back(m_colorImage.getView());
    if (m_depthImage.getView() != VK_NULL_HANDLE) {
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
        return false;
    }
    
    m_initialized = true;
    return true;
}

void OffscreenRenderer::destroy() {
    if (!m_initialized) return;
    
    if (m_framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
    }
    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    }
    
    m_colorImage.destroy();
    m_depthImage.destroy();
    
    m_initialized = false;
}

VkImage OffscreenRenderer::getColorImage() const {
    return m_colorImage.get();
}

VkImageView OffscreenRenderer::getColorImageView() const {
    return m_colorImage.getView();
}

VkRenderPass OffscreenRenderer::getRenderPass() const {
    return m_renderPass;
}

VkFramebuffer OffscreenRenderer::getFramebuffer() const {
    return m_framebuffer;
}

VkExtent2D OffscreenRenderer::getExtent() const {
    return {m_width, m_height};
}

void OffscreenRenderer::beginRenderPass(VkCommandBuffer cmd) const {
    VkRenderPassBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = m_renderPass;
    beginInfo.framebuffer = m_framebuffer;
    beginInfo.renderArea.offset = {0, 0};
    beginInfo.renderArea.extent = {m_width, m_height};
    
    std::vector<VkClearValue> clearValues;
    VkClearValue colorClear{};
    colorClear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues.push_back(colorClear);
    
    if (m_depthImage.get() != VK_NULL_HANDLE) {
        VkClearValue depthClear{};
        depthClear.depthStencil = {1.0f, 0};
        clearValues.push_back(depthClear);
    }
    
    beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    beginInfo.pClearValues = clearValues.data();
    
    vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void OffscreenRenderer::endRenderPass(VkCommandBuffer cmd) const {
    vkCmdEndRenderPass(cmd);
}

bool OffscreenRenderer::saveToPNG(const std::string& filename, VkQueue queue, VkCommandPool cmdPool) const {
    vk_utils::SaveImageInfo info{};
    info.device = m_device;
    info.physicalDevice = m_physicalDevice;
    info.queue = queue;
    info.commandPool = cmdPool;
    info.image = m_colorImage.get();
    info.format = VK_FORMAT_R8G8B8A8_SRGB;
    info.width = m_width;
    info.height = m_height;
    info.filename = filename;
    
    return vk_utils::saveVkImageToPNG(info);
}

} // namespace vk_offscreen