#include "file_utils.h"
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace vk_utils {

std::string getExecutableDirectory() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::filesystem::path exePath(path);
    return exePath.parent_path().string();
#else
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        std::filesystem::path exePath(path);
        return exePath.parent_path().string();
    }
    return ".";
#endif
}

std::string resolveResourcePath(const std::string& relativePath) {
    std::filesystem::path exeDir = getExecutableDirectory();
    std::filesystem::path fullPath = exeDir / relativePath;
    return fullPath.string();
}

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
    return resolveResourcePath("shaders");
}

std::string getShaderPath(const std::string& shaderName) {
    return resolveResourcePath("shaders/" + shaderName);
}

} // namespace vk_utils