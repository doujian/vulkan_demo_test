#include "file_utils.h"
#include <fstream>
#include <filesystem>

namespace vk_utils {

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        return {};
    }
    
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    
    return buffer;
}

bool fileExists(const std::string& filename) {
    return std::filesystem::exists(filename);
}

std::string getFileExtension(const std::string& filename) {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos) {
        return filename.substr(pos + 1);
    }
    return "";
}

std::string getShaderDirectory() {
    return "shaders";
}

std::string getShaderPath(const std::string& shaderName) {
    return getShaderDirectory() + "/" + shaderName;
}

} // namespace vk_utils