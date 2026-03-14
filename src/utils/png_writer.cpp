#include "png_writer.h"
#include "../core/buffer.h"
#include "../core/device.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <string.h>

#define STBIW_REALLOC(p,s) realloc(p,s)
#define STBIW_MALLOC(s) malloc(s)
#define STBIW_FREE(p) free(p)
#define STBIW_MEMMOVE(a,b,s) memmove(a,b,s)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../third_party/stb/stb_image_write.h"

namespace vk_utils {

bool saveImageToPNG(const std::string& filename, const uint8_t* data, 
                    uint32_t width, uint32_t height, uint32_t channels) {
    int result = stbi_write_png(filename.c_str(), width, height, channels, data, width * channels);
    return result != 0;
}

bool saveVkImageToPNG(const SaveImageInfo& info) {
    vk_core::Buffer buffer;
    
    if (!buffer.init(info.device, info.physicalDevice, info.width * info.height * 4,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        return false;
    }
    
    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandPool = info.commandPool;
    cmdAllocInfo.commandBufferCount = 1;
    
    VkCommandBuffer cmdBuffer;
    vkAllocateCommandBuffers(info.device, &cmdAllocInfo, &cmdBuffer);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = info.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, 
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {info.width, info.height, 1};
    
    vkCmdCopyImageToBuffer(cmdBuffer, info.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer.get(), 1, &region);
    
    vkEndCommandBuffer(cmdBuffer);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    
    vkQueueSubmit(info.queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(info.queue);
    
    vkFreeCommandBuffers(info.device, info.commandPool, 1, &cmdBuffer);
    
    void* mappedData = buffer.map();
    
    std::vector<uint8_t> imageData(info.width * info.height * 4);
    memcpy(imageData.data(), mappedData, info.width * info.height * 4);
    
    buffer.unmap();
    
    bool needSwapRB = (info.format == VK_FORMAT_B8G8R8A8_SRGB || 
                       info.format == VK_FORMAT_B8G8R8A8_UNORM);
    
    if (needSwapRB) {
        for (uint32_t i = 0; i < info.width * info.height; i++) {
            uint8_t temp = imageData[i * 4];
            imageData[i * 4] = imageData[i * 4 + 2];
            imageData[i * 4 + 2] = temp;
        }
    }
    
    bool result = saveImageToPNG(info.filename, imageData.data(), info.width, info.height, 4);
    
    return result;
}

} // namespace vk_utils