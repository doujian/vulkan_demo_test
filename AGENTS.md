# Agent Configuration

此文件存储编译和开发环境配置，供 AI 助手参考。

## 检查依赖
```
每次编译前，都检查依赖全不全。如果不全只允许调用setup_deps.py下载依赖，依赖没有满足前，不要尝试编译项目
```

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

## 代码风格

- 不添加注释（除非用户要求）
- 使用 4 空格缩进
- 命名空间: vk_demo, vk_core, vk_test, vk_utils, vk_offscreen
- 可能有错的异常分支都加上打印，用完不要删保留着。

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