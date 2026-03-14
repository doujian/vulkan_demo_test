#include "descriptor.h"

namespace vk_core {

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
    : m_device(other.m_device)
    , m_layout(other.m_layout)
    , m_initialized(other.m_initialized) {
    other.m_layout = VK_NULL_HANDLE;
    other.m_initialized = false;
}

DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_layout = other.m_layout;
        m_initialized = other.m_initialized;
        other.m_layout = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

DescriptorSetLayout::~DescriptorSetLayout() {
    destroy();
}

bool DescriptorSetLayout::init(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
    if (m_initialized) return true;
    
    m_device = device;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_layout) != VK_SUCCESS) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void DescriptorSetLayout::destroy() {
    if (!m_initialized) return;
    
    if (m_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr);
        m_layout = VK_NULL_HANDLE;
    }
    
    m_initialized = false;
}

DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
    : m_device(other.m_device)
    , m_pool(other.m_pool)
    , m_initialized(other.m_initialized) {
    other.m_pool = VK_NULL_HANDLE;
    other.m_initialized = false;
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_pool = other.m_pool;
        m_initialized = other.m_initialized;
        other.m_pool = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

DescriptorPool::~DescriptorPool() {
    destroy();
}

bool DescriptorPool::init(VkDevice device, const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets) {
    if (m_initialized) return true;
    
    m_device = device;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxSets;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_pool) != VK_SUCCESS) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void DescriptorPool::destroy() {
    if (!m_initialized) return;
    
    if (m_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_pool, nullptr);
        m_pool = VK_NULL_HANDLE;
    }
    
    m_initialized = false;
}

std::vector<VkDescriptorSet> DescriptorPool::allocate(VkDescriptorSetLayout layout, uint32_t count) {
    std::vector<VkDescriptorSetLayout> layouts(count, layout);
    std::vector<VkDescriptorSet> descriptorSets(count);
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_pool;
    allocInfo.descriptorSetCount = count;
    allocInfo.pSetLayouts = layouts.data();
    
    if (vkAllocateDescriptorSets(m_device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        return {};
    }
    
    return descriptorSets;
}

VkDescriptorSet DescriptorPool::allocate(VkDescriptorSetLayout layout) {
    auto sets = allocate(layout, 1);
    return sets.empty() ? VK_NULL_HANDLE : sets[0];
}

VkDescriptorSetLayoutBinding Descriptor::createUniformBinding(uint32_t binding, VkShaderStageFlags stageFlags) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = stageFlags;
    layoutBinding.pImmutableSamplers = nullptr;
    return layoutBinding;
}

VkDescriptorSetLayoutBinding Descriptor::createSamplerBinding(uint32_t binding, VkShaderStageFlags stageFlags) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = stageFlags;
    layoutBinding.pImmutableSamplers = nullptr;
    return layoutBinding;
}

VkDescriptorSetLayoutBinding Descriptor::createStorageImageBinding(uint32_t binding, VkShaderStageFlags stageFlags) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = stageFlags;
    layoutBinding.pImmutableSamplers = nullptr;
    return layoutBinding;
}

VkDescriptorPoolSize Descriptor::createPoolSize(VkDescriptorType type, uint32_t count) {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = type;
    poolSize.descriptorCount = count;
    return poolSize;
}

} // namespace vk_core