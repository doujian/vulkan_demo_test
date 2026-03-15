@echo off
setlocal

echo ========================================
echo Vulkan Demo Test - Build Script
echo ========================================
echo.

set PROJECT_DIR=%~dp0
set BUILD_DIR=%PROJECT_DIR%build
set MSBUILD="D:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"

echo [1/4] Checking dependencies...
python "%PROJECT_DIR%setup_deps.py"
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Dependency setup failed!
    pause
    exit /b 1
)

echo.
echo [2/4] Configuring CMake...
cmake -B "%BUILD_DIR%" -G "Visual Studio 18 2026" -A x64
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo [3/4] Building Release version...
%MSBUILD% "%BUILD_DIR%\vulkan_demo_test.slnx" /p:Configuration=Release /m
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo [4/4] Running tests...
"%BUILD_DIR%\bin\Release\vulkan_demo.exe" --run-all-tests
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Tests failed!
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