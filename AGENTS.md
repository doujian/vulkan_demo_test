# Agent Configuration

此文件存储编译和开发环境配置，供 AI 助手参考。

## 构建脚本

项目提供便捷的构建脚本：

```
build.bat           - Windows 版本构建（自动检查依赖）
build_android_cli.bat - Android CLI 版本构建
build_android_apk.bat - Android APK 版本构建
```

## 检查依赖
```
每次编译前，都检查依赖全不全。如果不全只允许调用setup_deps.py下载依赖，依赖没有满足前，不要尝试编译项目
```

setup_deps.py 会自动下载：
- glm
- tinygltf
- stb
- ktx
- JDK 17 (用于 Android APK 构建)

## 单元测试
```
禁止你主动使用 --update-golden, 当我输入update-golden的时候，你才可以
```

## 编译工具路径

```
MSBuild: D:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe
Visual Studio: D:\Program Files\Microsoft Visual Studio\18\Community
```

## 编译命令

```bash
# 使用 MSBuild 编译
"D:/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe" "D:/Projects/vulkan_demo_test/build/vulkan_demo_test.slnx" //p:Configuration=Release //m

# 或在工作目录下
cd D:/Projects/vulkan_demo_test/build
"/d/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe" vulkan_demo_test.slnx //p:Configuration=Release //m
```

## 输出路径

```
可执行文件: D:\Projects\vulkan_demo_test\build\bin\Release\vulkan_demo.exe
```

## Vulkan SDK

```
路径: E:/VulkanSDK/1.4.341.1
如果不在上面路径，找到后修改。
```

## Android SDK/NDK

```
Android SDK: E:/Android/Sdk
Android NDK: E:/Android/Sdk/ndk/29.0.14206865
Ninja: E:/Android/Sdk/cmake/4.1.2/bin/ninja.exe
Android Studio: D:/Program Files/Android/Android Studio
Gradle: 项目自带 (gradle/gradle-8.5)
JDK 17: 项目自带 (jdk/jdk-17.0.2) - APK 构建必须用 JDK 17
```

## Android 编译命令

```bash
# Android CLI 版本编译
cmake -B "android/build_cli" -S "android" -G "Ninja" \
  -DCMAKE_TOOLCHAIN_FILE="E:/Android/Sdk/ndk/29.0.14206865/build/cmake/android.toolchain.cmake" \
  -DCMAKE_MAKE_PROGRAM="E:/Android/Sdk/cmake/4.1.2/bin/ninja.exe" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DANDROID_STL=c++_static \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_VALIDATION=OFF \
  -DBUILD_ANDROID_CLI=ON \
  -DBUILD_ANDROID_APP=OFF

cmake --build "android/build_cli" --config Release

# Android APK 版本编译 (需要 JDK 17)
# 1. 先编译 native 库
cmake -B "android/build_apk" -S "android" -G "Ninja" \
  -DCMAKE_TOOLCHAIN_FILE="E:/Android/Sdk/ndk/29.0.14206865/build/cmake/android.toolchain.cmake" \
  -DCMAKE_MAKE_PROGRAM="E:/Android/Sdk/cmake/4.1.2/bin/ninja.exe" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DANDROID_STL=c++_static \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_VALIDATION=OFF \
  -DBUILD_ANDROID_CLI=OFF \
  -DBUILD_ANDROID_APP=ON

cmake --build "android/build_apk" --config Release

# 2. 复制 .so 到 jniLibs
mkdir -p android/apk/app/src/main/jniLibs/arm64-v8a
cp android/build_apk/libvulkan_demo.so android/apk/app/src/main/jniLibs/arm64-v8a/

# 3. 复制资源
cp -r shaders android/apk/app/src/main/assets/
cp -r golden android/apk/app/src/main/assets/

# 4. 用 Gradle 构建 APK (必须用 JDK 17)
export JAVA_HOME="D:/Projects/temp/temp2/vulkan_demo_test/jdk/jdk-17.0.2"
cd android/apk
../../gradle/gradle-8.5/bin/gradle assembleRelease

# 5. 签名 APK
JAVA_HOME="../../jdk/jdk-17.0.2" E:/Android/Sdk/build-tools/36.1.0/zipalign.exe -v 4 \
  app/build/outputs/apk/release/app-release-unsigned.apk app-aligned.apk
JAVA_HOME="../../jdk/jdk-17.0.2" E:/Android/Sdk/build-tools/36.1.0/apksigner.bat sign \
  --ks release-key.jks --ks-pass pass:android --key-pass pass:android \
  --out app-release.apk app-aligned.apk
```

输出:
- CLI: android/build_cli/vulkan_demo
- APK: android/apk/app-release.apk

## Android 部署命令

```bash
# 推送 CLI 版本到设备
adb push android/build_cli/vulkan_demo /data/local/tmp/
adb push shaders /data/local/tmp/
adb push golden /data/local/tmp/
adb shell chmod +x /data/local/tmp/vulkan_demo

# 运行 Android CLI 测试
adb shell /data/local/tmp/vulkan_demo --run-all-tests

# 安装 APK
adb install android/apk/app-release.apk
```

## 代码风格

- 不添加注释（除非用户要求）
- 使用 4 空格缩进
- 命名空间: vk_demo, vk_core, vk_test, vk_utils, vk_offscreen
- 可能有错的异常分支都加上打印，用完不要删保留着。

## 平台代码

`src/app.cpp` 使用 `#ifdef __ANDROID__` 宏隔离平台差异：
- Windows: 使用 GLFW 创建窗口，支持窗口模式和离屏模式
- Android: 仅支持离屏模式，使用 android log 输出

不要创建 `app_android.cpp`，平台差异在 `app.cpp` 中用宏隔离。

## 测试命令

```bash
# 生成 golden 图像
vulkan_demo.exe -d triangle --offscreen --update-golden

# 运行测试
vulkan_demo.exe -d triangle --offscreen --test
```

## 更新README
```
每次完成任务后更新README.md
```

## 修复错误
```
跑一下validation layer
```