#pragma once

#include <string>
#include <vector>

namespace vk_utils {

struct Args {
    std::string demoName = "triangle";
    bool validation = false;
    uint32_t frameCount = 0;
    std::string outputPath = "output";
    std::string goldenPath = "golden";
    uint32_t width = 800;
    uint32_t height = 600;
    bool offscreen = false;
    bool listDemos = false;
    bool testMode = false;
    bool updateGolden = false;
    bool runAllTests = false;
    float testThreshold = 0.99f;
};

class ArgsParser {
public:
    static Args parse(int argc, char* argv[]);
    static void printUsage(const char* programName);
};

} // namespace vk_utils