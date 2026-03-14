#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace vk_core {

class Shader {
public:
    static std::vector<char> readFile(const std::string& filename);
    static VkShaderModule createModule(VkDevice device, const std::vector<char>& code);
};

} // namespace vk_core