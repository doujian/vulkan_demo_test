@echo off
setlocal EnableDelayedExpansion

echo ========================================
echo Vulkan Demo - Android CLI Build
echo ========================================
echo.

set PROJECT_DIR=%~dp0
set NDK_PATH=E:/Android/Sdk/ndk/29.0.14206865
set NINJA_PATH=E:/Android/Sdk/cmake/4.1.2/bin/ninja.exe
set BUILD_DIR=%PROJECT_DIR%android\build_cli

echo [1/2] Configuring CMake...
cmake -B "%BUILD_DIR%" -S "%PROJECT_DIR%android" -G "Ninja" ^
  -DCMAKE_TOOLCHAIN_FILE="%NDK_PATH%/build/cmake/android.toolchain.cmake" ^
  -DCMAKE_MAKE_PROGRAM="%NINJA_PATH%" ^
  -DANDROID_ABI=arm64-v8a ^
  -DANDROID_PLATFORM=android-26 ^
  -DANDROID_STL=c++_static ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DENABLE_VALIDATION=OFF ^
  -DBUILD_ANDROID_CLI=ON ^
  -DBUILD_ANDROID_APP=OFF

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo [2/2] Building...
cmake --build "%BUILD_DIR%" --config Release
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo Executable: %BUILD_DIR%\vulkan_demo
echo ========================================
echo.
pause