@echo off
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set PROJECT_DIR=%SCRIPT_DIR%..\..
set ANDROID_BUILD_DIR=%PROJECT_DIR%\android\build_cli
set ANDROID_NDK_PATH=
set ABI=arm64-v8a

if defined ANDROID_NDK_HOME (
    set ANDROID_NDK_PATH=%ANDROID_NDK_HOME%
) else if defined NDK_HOME (
    set ANDROID_NDK_PATH=%NDK_HOME%
) else (
    echo ERROR: ANDROID_NDK_HOME or NDK_HOME not set
    echo Please set the environment variable to your Android NDK path
    exit /b 1
)

echo Using NDK: %ANDROID_NDK_PATH%
echo Building for ABI: %ABI%

if exist "%ANDROID_BUILD_DIR%" rmdir /s /q "%ANDROID_BUILD_DIR%"
mkdir "%ANDROID_BUILD_DIR%"

cmake -B "%ANDROID_BUILD_DIR%" ^
    -S "%PROJECT_DIR%\android" ^
    -G "Ninja" ^
    -DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK_PATH%\build\cmake\android.toolchain.cmake" ^
    -DANDROID_ABI=%ABI% ^
    -DANDROID_PLATFORM=android-26 ^
    -DANDROID_STL=c++_static ^
    -DANDROID_ARM_MODE=arm ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DENABLE_VALIDATION=OFF ^
    -DBUILD_ANDROID_CLI=ON ^
    -DBUILD_ANDROID_APP=OFF

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed
    exit /b 1
)

cmake --build "%ANDROID_BUILD_DIR%" --config Release -j

if %ERRORLEVEL% neq 0 (
    echo Build failed
    exit /b 1
)

echo.
echo Build successful!
echo Output: %ANDROID_BUILD_DIR%\vulkan_demo

endlocal