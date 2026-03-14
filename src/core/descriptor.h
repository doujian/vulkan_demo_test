#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace vk_core {

class DescriptorSetLayout {
public:
    DescriptorSetLayout() = default;
    ~DescriptorSetLayout();
    
    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout(DescriptorSetLayout&&) noexcept;
    DescriptorSetLayout& operator=(DescriptorSetLayout&&) noexcept;
    
    bool init(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings);
    void destroy();
    
    VkDescriptorSetLayout get() const { return m_layout; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
    bool m_initialized = false;
};

class DescriptorPool {
public:
    DescriptorPool() = default;
    ~DescriptorPool();
    
    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;
    DescriptorPool(DescriptorPool&&) noexcept;
    DescriptorPool& operator=(DescriptorPool&&) noexcept;
    
    bool init(VkDevice device, const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);
    void destroy();
    
    std::vector<VkDescriptorSet> allocate(VkDescriptorSetLayout layout, uint32_t count);
    VkDescriptorSet allocate(VkDescriptorSetLayout layout);
    
    VkDescriptorPool get() const { return m_pool; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkDescriptorPool m_pool = VK_NULL_HANDLE;
    bool m_initialized = false;
};

class Descriptor {
public:
    static VkDescriptorSetLayoutBinding createUniformBinding(uint32_t binding, VkShaderStageFlags stageFlags);
    static VkDescriptorSetLayoutBinding createSamplerBinding(uint32_t binding, VkShaderStageFlags stageFlags);
    static VkDescriptorSetLayoutBinding createStorageImageBinding(uint32_t binding, VkShaderStageFlags stageFlags);
    static VkDescriptorPoolSize createPoolSize(VkDescriptorType type, uint32_t count);
};

} // namespace vk_core