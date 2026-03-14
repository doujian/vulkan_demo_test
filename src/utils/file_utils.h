#pragma once

#include <string>
#include <vector>

namespace vk_utils {

std::vector<char> readFile(const std::string& filename);
bool fileExists(const std::string& filename);
std::string getFileExtension(const std::string& filename);
std::string getShaderDirectory();
std::string getShaderPath(const std::string& shaderName);

} // namespace vk_utils