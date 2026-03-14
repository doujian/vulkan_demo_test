#include "args_parser.h"
#include <iostream>
#include <cstring>

namespace vk_utils {

Args ArgsParser::parse(int argc, char* argv[]) {
    Args args;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--demo") == 0) {
            if (i + 1 < argc) {
                args.demoName = argv[++i];
            }
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--validation") == 0) {
            args.validation = true;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--frames") == 0) {
            if (i + 1 < argc) {
                args.frameCount = std::stoul(argv[++i]);
            }
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                args.outputPath = argv[++i];
            }
        } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0) {
            if (i + 1 < argc) {
                args.width = std::stoul(argv[++i]);
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--height") == 0) {
            if (i + 1 < argc) {
                args.height = std::stoul(argv[++i]);
            }
        } else if (strcmp(argv[i], "--offscreen") == 0) {
            args.offscreen = true;
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0) {
            args.listDemos = true;
        } else if (strcmp(argv[i], "--test") == 0) {
            args.testMode = true;
            args.offscreen = true;
        } else if (strcmp(argv[i], "--run-all-tests") == 0) {
            args.runAllTests = true;
            args.testMode = true;
            args.offscreen = true;
        } else if (strcmp(argv[i], "--update-golden") == 0) {
            args.updateGolden = true;
            args.offscreen = true;
        } else if (strcmp(argv[i], "--golden") == 0) {
            if (i + 1 < argc) {
                args.goldenPath = argv[++i];
            }
        } else if (strcmp(argv[i], "--threshold") == 0) {
            if (i + 1 < argc) {
                args.testThreshold = std::stof(argv[++i]);
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            exit(0);
        }
    }
    
    return args;
}

void ArgsParser::printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  -d, --demo <name>      Demo to run (default: triangle)\n"
              << "  -v, --validation       Enable validation layers\n"
              << "  -n, --frames <count>   Number of frames (default: 0=unlimited)\n"
              << "  -o, --output <path>    Output directory (default: output)\n"
              << "  -w, --width <pixels>   Width (default: 800)\n"
              << "  -h, --height <pixels>  Height (default: 600)\n"
              << "  --offscreen            Run in offscreen mode\n"
              << "  -l, --list             List available demos\n"
              << "  --test                 Run in test mode (compare with golden)\n"
              << "  --run-all-tests        Run tests for all demos\n"
              << "  --update-golden        Update golden images\n"
              << "  --golden <path>        Golden directory (default: golden)\n"
              << "  --threshold <value>    Test similarity threshold (default: 0.99)\n"
              << "  --help                 Show this help message\n";
}

} // namespace vk_utils