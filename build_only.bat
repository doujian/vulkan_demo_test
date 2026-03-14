@echo off
setlocal

echo ========================================
echo Vulkan Demo Test - Build Only
echo ========================================
echo.

set PROJECT_DIR=%~dp0
set BUILD_DIR=%PROJECT_DIR%build
set MSBUILD="D:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"

if exist "%BUILD_DIR%" (
    echo Found existing build directory, rebuilding...
) else (
    echo [1/2] Configuring CMake...
    cmake -B "%BUILD_DIR%" -G "Visual Studio 18 2026" -A x64
    if %errorlevel% neq 0 (
        echo.
        echo [ERROR] CMake configuration failed!
        pause
        exit /b 1
    )
)

echo.
echo [2/2] Building Release version...
%MSBUILD% "%BUILD_DIR%\vulkan_demo_test.slnx" /p:Configuration=Release /m /v:m
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo Executable: %BUILD_DIR%\bin\Release\vulkan_demo.exe
echo ========================================
echo.
pause