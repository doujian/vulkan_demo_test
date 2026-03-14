#pragma once

#include "image_comparator.h"
#include <string>
#include <vector>
#include <functional>

namespace vk_test {

struct TestResult {
    std::string testName;
    bool passed = false;
    std::string message;
};

class TestRunner {
public:
    using RenderFunc = std::function<std::string()>;
    
    TestRunner(const std::string& goldenDir, const std::string& outputDir);
    
    TestResult runTest(const std::string& demoName, RenderFunc renderFunc,
                       uint32_t frameIndex, float threshold = 0.99f);
    
    void updateGolden(const std::string& demoName, const std::string& renderedImage,
                      uint32_t frameIndex);
    
    std::string getGoldenPath(const std::string& demoName, uint32_t frameIndex) const;
    
    bool hasGolden(const std::string& demoName, uint32_t frameIndex) const;

private:
    std::string m_goldenDir;
    std::string m_outputDir;
};

} // namespace vk_test