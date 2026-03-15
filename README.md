# Vulkan Demo Test

基于 Vulkan 的图形渲染演示框架，支持窗口模式和离屏渲染。

## 功能特性

- **多种渲染模式**：支持窗口实时渲染和离屏渲染
- **Demo 注册机制**：插件式架构，方便扩展新的演示用例
- **自动截图保存**：离屏模式自动保存 PNG 图片，带时间戳目录
- **Validation 日志**：支持 Vulkan 验证层，日志自动保存

## 目录结构

```
vulkan_demo_test/
├── src/
│   ├── main.cpp              # 入口
│   ├── app.h/cpp             # 应用主逻辑
│   ├── core/                 # Vulkan 核心封装
│   │   ├── instance.h/cpp
│   │   ├── device.h/cpp
│   │   ├── swapchain.h/cpp
│   │   ├── pipeline.h/cpp
│   │   └── ...
│   ├── demo/                 # Demo 系统
│   │   ├── demo_base.h       # Demo 基类
│   │   ├── demo_registry.h   # Demo 注册器
│   │   └── demos/            # Demo 实现
│   │       ├── triangle/
│   │       └── multi_pass_blend/
│   ├── offscreen/            # 离屏渲染
│   └── utils/                # 工具类
├── shaders/                  # 着色器
│   ├── triangle/
│   ├── multi_pass_blend/
│   └── *.spv
├── assets/                   # 资源文件
│   ├── models/               # glTF 模型
│   └── textures/             # 纹理
├── third_party/              # 第三方库
│   ├── tinygltf/
│   ├── ktx/
│   └── glm/
├── CMakeLists.txt
└── README.md
```

## 编译

### Windows 环境

- CMake 3.16+
- C++17 编译器
- Vulkan SDK 1.2+
- GLFW (自动下载)

### 快速编译

```bash
# 运行构建脚本（自动检查并下载依赖）
build.bat
```

构建脚本会自动：
1. 检查并下载缺失的依赖（glm、tinygltf、stb、ktx、JDK 17）
2. 配置 CMake
3. 编译 Release 版本
4. 运行测试

### 手动编译

```bash
# 先下载依赖
python setup_deps.py

# 配置
cmake -B build -G "Visual Studio 18 2026" -A x64

# 编译
cmake --build build --config Release
```

编译完成后，`assets/`、`shaders/`、`golden/` 目录会自动复制到输出目录。

### Vulkan SDK 路径

默认路径：`E:/VulkanSDK/1.4.341.1`

修改 `CMakeLists.txt` 中的 `VULKAN_SDK` 变量以更改路径。

## Android 编译

### 环境要求

- Android NDK r25+
- CMake 3.22+
- Ninja 构建工具
- Android SDK (APK 版本需要)
- Gradle 8.0+ (APK 版本需要)
- JDK 17 (APK 版本需要，项目自带)

### 快速编译

```bash
# Android CLI 版本
build_android_cli.bat

# Android APK 版本
build_android_apk.bat
```

构建脚本会自动：
1. 检查并下载缺失的依赖
2. 编译 native 库
3. 复制资源和 .so 文件
4. 构建 APK 并签名（APK 版本）

### Android CLI 版本 (离屏渲染测试)

适用于 Android 12+ 设备的命令行版本，支持离屏渲染和测试：

```bash
# 设置 NDK 路径
set ANDROID_NDK_HOME=path/to/android-ndk

# 构建
cd android
build_cli.bat
```

编译完成后，可执行文件位于：`android/build_cli/vulkan_demo`

### 部署和运行 (CLI 版本)

```bash
# 推送到设备
adb push android/build_cli/vulkan_demo /data/local/tmp/
adb push shaders /data/local/tmp/
adb push golden /data/local/tmp/
adb shell chmod +x /data/local/tmp/vulkan_demo

# 运行测试
adb shell /data/local/tmp/vulkan_demo --run-all-tests

# 运行单个 demo
adb shell /data/local/tmp/vulkan_demo -d triangle --offscreen -n 10 -o /sdcard/vulkan_output
```

### Android APK 版本

适用于在手机上实时显示渲染结果：

```bash
cd android
build_apk.bat
```

编译完成后，APK 位于：`android/apk/app/build/outputs/apk/release/app-release.apk`

### 安装和运行 (APK 版本)

```bash
# 安装 APK
adb install android/apk/app/build/outputs/apk/release/app-release.apk

# 启动应用
adb shell am start -n com.vulkandemo/.MainActivity
```

## 使用方法

### 命令行参数

```
vulkan_demo.exe [选项]

选项:
  -d, --demo <name>      指定运行的 demo (默认: triangle)
  -n, --frames <n>       渲染帧数 (默认: 0=无限循环)
  -w, --width <w>        窗口宽度 (默认: 800)
  -h, --height <h>       窗口高度 (默认: 600)
  --offscreen            离屏渲染模式
  -o, --output <path>    输出目录 (默认: output)
  -v, --validation       启用 Vulkan 验证层
  -l, --list             列出所有可用的 demo
  --test                 测试模式，与 golden 图像比较
  --run-all-tests        运行所有 demo 的测试
  --update-golden        更新 golden 参考图像
  --golden <path>        golden 目录 (默认: golden)
  --threshold <value>    测试相似度阈值 (默认: 0.99)
```

### 示例

```bash
# 列出所有 demo
vulkan_demo.exe -l

# 窗口模式运行 triangle demo
vulkan_demo.exe -d triangle

# 窗口模式运行 100 帧后退出
vulkan_demo.exe -d triangle -n 100

# 离屏渲染 10 帧
vulkan_demo.exe -d triangle --offscreen -n 10

# 启用验证层
vulkan_demo.exe -d triangle -v

# 离屏渲染 + 验证层
vulkan_demo.exe -d triangle --offscreen -n 10 -v
```

### 测试模式

测试模式用于验证渲染结果是否与预期一致：

```bash
# 1. 首次运行，生成 golden 参考图像
vulkan_demo.exe -d triangle --offscreen --update-golden

# 2. 后续运行测试，比较渲染结果与 golden
vulkan_demo.exe -d triangle --offscreen --test

# 3. 运行所有 demo 的测试
vulkan_demo.exe --run-all-tests
```

**工作流程：**

| 模式 | 行为 |
|------|------|
| `--update-golden` | 渲染后保存图像到 `golden/` 目录作为参考 |
| `--test` | 渲染后与 golden 比较，输出 PASS/FAIL |

**测试输出示例：**
```
[triangle_0] PASS (similarity: 100.000000%)

=== Test Summary ===
Passed: 1/1
Failed: 0/1
```

**运行所有测试示例：**
```
--- Testing demo: triangle ---
  Frame 0: PASS (similarity: 100.000000%)
  [PASS] triangle: 1/1 frames passed

=== Test Summary ===
Passed: 1/1 demos
Failed: 0/1 demos
```

### 输出结果

**离屏模式**：
```
output/
└── triangle_20260314_160837/
    ├── 0000.png
    ├── 0001.png
    ├── ...
    └── validation.txt    # 如果有验证层警告
```

**窗口模式**：
```
output/
└── triangle_20260314_160837/
    └── validation.txt    # 如果有验证层警告
```

## 添加新的 Demo

1. 在 `src/demo/demos/` 下创建新目录

2. 实现 Demo 类：

```cpp
#include "demo/demo_base.h"
#include "demo/demo_registry.h"

namespace vk_demo {

class DemoMyDemo : public DemoBase {
public:
    std::string getName() const override { return "mydemo"; }
    std::string getDescription() const override { return "My custom demo"; }
    
    bool init(const DemoContext& context) override {
        m_context = context;
        // 初始化资源...
        return true;
    }
    
    void destroy() override {
        // 清理资源...
    }
    
    void recordCommandBuffer(VkCommandBuffer cmd, VkFramebuffer fb) override {
        // 记录渲染命令...
    }

private:
    DemoContext m_context;
};

REGISTER_DEMO(DemoMyDemo, "mydemo", "My custom demo")

} // namespace vk_demo
```

3. 更新 `src/demo/demos/CMakeLists.txt`

## Demo 配置

每个 Demo 可以自定义配置：

```cpp
DemoConfig getConfig() const override {
    DemoConfig config;
    config.width = 1920;
    config.height = 1080;
    config.enableDepth = true;
    return config;
}
```

### DemoConfig 选项

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| width | uint32_t | 800 | 渲染宽度 |
| height | uint32_t | 600 | 渲染高度 |
| enableDepth | bool | true | 是否启用深度缓冲 |
| colorFormat | VkFormat | B8G8R8A8_SRGB | 颜色格式 |
| clearColor | VkClearColorValue | {0,0,0,1} | 清屏颜色 |

## 架构设计

### App 类

主应用类，负责：
- 窗口管理 (GLFW)
- Vulkan 初始化
- 渲染循环
- 资源清理

### DemoBase 接口

Demo 基类，定义：
- `init()`: 初始化资源
- `destroy()`: 清理资源
- `recordCommandBuffer()`: 记录渲染命令
- `update()`: 帧更新逻辑

### DemoRegistry

Demo 注册器，支持：
- 运行时注册 Demo
- 按名称创建 Demo 实例
- 列出所有已注册的 Demo

## 同步模型

采用双缓冲 (Double Buffering) 同步：

```
Fence:       2个 (CPU-GPU 同步)
CommandBuffer: 2个 (与 Fence 对应)
ImageAvailable Semaphore: 2个 (有 Fence 保护)
RenderFinished Semaphore: N个 (每个 Swapchain 图像独立)
```

## 依赖

- [Vulkan](https://www.vulkan.org/) - 图形 API
- [GLFW](https://www.glfw.org/) - 窗口管理
- [stb_image_write](https://github.com/nothings/stb) - PNG 写入
- [tinygltf](https://github.com/syoyo/tinygltf) - glTF 模型加载
- [KTX](https://github.com/KhronosGroup/KTX-Software) - KTX 纹理加载
- [glm](https://github.com/g-truc/glm) - 数学库

## Demos

### triangle
简单的彩色三角形渲染。

### multi_pass_blend
多通道混合演示。



## License

MIT