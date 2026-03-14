#pragma once

#include <string>
#include <vector>

namespace vk_utils {

std::vector<char> readFile(const std::string& filename);
bool fileExists(const std::string& filename);
std::string getFileExtension(const std::string& filename);
std::string getExecutableDirectory();
std::string getShaderDirectory();
std::string getShaderPath(const std::string& shaderName);
std::string resolveResourcePath(const std::string& relativePath);

} // namespace vk_utils