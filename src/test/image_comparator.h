#pragma once

#include <string>
#include <cstdint>

namespace vk_test {

struct CompareResult {
    bool passed = false;
    float similarity = 0.0f;
    uint32_t diffPixels = 0;
    uint32_t totalPixels = 0;
    std::string message;
};

class ImageComparator {
public:
    static CompareResult compare(const std::string& image1, const std::string& image2,
                                  float threshold = 0.99f);
    
    static bool fileExists(const std::string& path);
    
    static bool copyFile(const std::string& src, const std::string& dst);
};

} // namespace vk_test