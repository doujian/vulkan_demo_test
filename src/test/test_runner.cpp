#include "test_runner.h"
#include <iostream>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <direct.h>
#endif

namespace vk_test {

TestRunner::TestRunner(const std::string& goldenDir, const std::string& outputDir)
    : m_goldenDir(goldenDir)
    , m_outputDir(outputDir) {
}

std::string TestRunner::getGoldenPath(const std::string& demoName, uint32_t frameIndex) const {
    std::ostringstream ss;
    ss << m_goldenDir << "/" << demoName << "_" << std::setfill('0') << std::setw(4) << frameIndex << ".png";
    return ss.str();
}

bool TestRunner::hasGolden(const std::string& demoName, uint32_t frameIndex) const {
    return ImageComparator::fileExists(getGoldenPath(demoName, frameIndex));
}

void TestRunner::updateGolden(const std::string& demoName, const std::string& renderedImage,
                               uint32_t frameIndex) {
#ifdef _WIN32
    _mkdir(m_goldenDir.c_str());
#endif
    
    std::string goldenPath = getGoldenPath(demoName, frameIndex);
    if (ImageComparator::copyFile(renderedImage, goldenPath)) {
        std::cout << "Golden updated: " << goldenPath << "\n";
    } else {
        std::cerr << "Failed to update golden: " << goldenPath << "\n";
    }
}

TestResult TestRunner::runTest(const std::string& demoName, RenderFunc renderFunc,
                                uint32_t frameIndex, float threshold) {
    TestResult result;
    result.testName = demoName + "_" + std::to_string(frameIndex);
    
    std::string renderedImage = renderFunc();
    if (renderedImage.empty()) {
        result.message = "Render failed";
        return result;
    }
    
    std::string goldenPath = getGoldenPath(demoName, frameIndex);
    
    if (!hasGolden(demoName, frameIndex)) {
        updateGolden(demoName, renderedImage, frameIndex);
        result.message = "Golden created (first run)";
        result.passed = true;
        return result;
    }
    
    CompareResult compareResult = ImageComparator::compare(goldenPath, renderedImage, threshold);
    
    result.passed = compareResult.passed;
    result.message = compareResult.message;
    
    return result;
}

} // namespace vk_test