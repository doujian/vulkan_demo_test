#include "image_comparator.h"
#include <fstream>
#include <vector>
#include <cmath>

#include "stb/stb_image.h"

namespace vk_test {

bool ImageComparator::fileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

bool ImageComparator::copyFile(const std::string& src, const std::string& dst) {
    std::ifstream in(src, std::ios::binary);
    std::ofstream out(dst, std::ios::binary);
    if (!in || !out) return false;
    out << in.rdbuf();
    return true;
}

CompareResult ImageComparator::compare(const std::string& image1, const std::string& image2,
                                        float threshold) {
    CompareResult result;
    
    int w1, h1, c1;
    int w2, h2, c2;
    
    stbi_set_flip_vertically_on_load(0);
    
    unsigned char* data1 = stbi_load(image1.c_str(), &w1, &h1, &c1, 4);
    unsigned char* data2 = stbi_load(image2.c_str(), &w2, &h2, &c2, 4);
    
    if (!data1) {
        result.message = "Failed to load: " + image1;
        stbi_image_free(data2);
        return result;
    }
    
    if (!data2) {
        result.message = "Failed to load: " + image2;
        stbi_image_free(data1);
        return result;
    }
    
    if (w1 != w2 || h1 != h2) {
        result.message = "Image size mismatch: " + 
                         std::to_string(w1) + "x" + std::to_string(h1) + 
                         " vs " + std::to_string(w2) + "x" + std::to_string(h2);
        stbi_image_free(data1);
        stbi_image_free(data2);
        return result;
    }
    
    result.totalPixels = w1 * h1;
    uint32_t matchPixels = 0;
    const uint32_t tolerance = 5;
    
    for (int i = 0; i < w1 * h1 * 4; i += 4) {
        bool pixelMatch = true;
        for (int j = 0; j < 4; j++) {
            int diff = std::abs(static_cast<int>(data1[i + j]) - static_cast<int>(data2[i + j]));
            if (diff > tolerance) {
                pixelMatch = false;
                break;
            }
        }
        if (pixelMatch) {
            matchPixels++;
        }
    }
    
    stbi_image_free(data1);
    stbi_image_free(data2);
    
    result.diffPixels = result.totalPixels - matchPixels;
    result.similarity = static_cast<float>(matchPixels) / result.totalPixels;
    result.passed = result.similarity >= threshold;
    
    if (result.passed) {
        result.message = "PASS (similarity: " + std::to_string(result.similarity * 100) + "%)";
    } else {
        result.message = "FAIL (similarity: " + std::to_string(result.similarity * 100) + 
                         "%, diff pixels: " + std::to_string(result.diffPixels) + ")";
    }
    
    return result;
}

} // namespace vk_test