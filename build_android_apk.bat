@echo off
setlocal EnableDelayedExpansion

echo ========================================
echo Vulkan Demo - Android APK Build
echo ========================================
echo.

set PROJECT_DIR=%~dp0
set NDK_PATH=E:/Android/Sdk/ndk/29.0.14206865
set NINJA_PATH=E:/Android/Sdk/cmake/4.1.2/bin/ninja.exe
set SDK_PATH=E:/Android/Sdk
set BUILD_TOOLS=%SDK_PATH%/build-tools/36.1.0
set JAVA_HOME=%PROJECT_DIR%jdk\jdk-17.0.2
set GRADLE_PATH=%PROJECT_DIR%gradle\gradle-8.5\bin\gradle
set APK_DIR=%PROJECT_DIR%android\apk
set NATIVE_BUILD_DIR=%PROJECT_DIR%android\build_apk

echo [1/5] Configuring CMake for native library...
cmake -B "%NATIVE_BUILD_DIR%" -S "%PROJECT_DIR%android" -G "Ninja" ^
  -DCMAKE_TOOLCHAIN_FILE="%NDK_PATH%/build/cmake/android.toolchain.cmake" ^
  -DCMAKE_MAKE_PROGRAM="%NINJA_PATH%" ^
  -DANDROID_ABI=arm64-v8a ^
  -DANDROID_PLATFORM=android-26 ^
  -DANDROID_STL=c++_static ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DENABLE_VALIDATION=OFF ^
  -DBUILD_ANDROID_CLI=OFF ^
  -DBUILD_ANDROID_APP=ON

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo [2/5] Building native library...
cmake --build "%NATIVE_BUILD_DIR%" --config Release
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Native build failed!
    pause
    exit /b 1
)

echo.
echo [3/5] Copying files...
if not exist "%APK_DIR%\app\src\main\jniLibs\arm64-v8a" mkdir "%APK_DIR%\app\src\main\jniLibs\arm64-v8a"
copy /Y "%NATIVE_BUILD_DIR%\libvulkan_demo.so" "%APK_DIR%\app\src\main\jniLibs\arm64-v8a\" >nul

if not exist "%APK_DIR%\app\src\main\assets" mkdir "%APK_DIR%\app\src\main\assets"
xcopy /E /Y /I "%PROJECT_DIR%shaders" "%APK_DIR%\app\src\main\assets\shaders" >nul
xcopy /E /Y /I "%PROJECT_DIR%golden" "%APK_DIR%\app\src\main\assets\golden" >nul

echo.
echo [4/5] Building APK with Gradle...
cd /d "%APK_DIR%"
set ANDROID_HOME=%SDK_PATH%
set JAVA_HOME=%JAVA_HOME%
call "%GRADLE_PATH%" assembleRelease --no-daemon
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Gradle build failed!
    cd /d "%PROJECT_DIR%"
    pause
    exit /b 1
)

echo.
echo [5/5] Signing APK...
if exist app-aligned.apk del /f /q app-aligned.apk
"%BUILD_TOOLS%/zipalign.exe" -v 4 app\build\outputs\apk\release\app-release-unsigned.apk app-aligned.apk
if %errorlevel% neq 0 (
    echo [ERROR] zipalign failed!
    cd /d "%PROJECT_DIR%"
    pause
    exit /b 1
)

call "%BUILD_TOOLS%/apksigner.bat" sign --ks release-key.jks --ks-pass pass:android --key-pass pass:android --out app-release.apk app-aligned.apk 2>&1
set SIGN_RESULT=%errorlevel%
if %SIGN_RESULT% neq 0 (
    echo [ERROR] APK signing failed!
    cd /d "%PROJECT_DIR%"
    pause
    exit /b 1
)

del /f /q app-aligned.apk >nul 2>&1
cd /d "%PROJECT_DIR%"

echo.
echo ========================================
echo Build completed successfully!
echo APK: %APK_DIR%\app-release.apk
echo ========================================
echo.
pause