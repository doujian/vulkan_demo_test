@echo off
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set PROJECT_DIR=%SCRIPT_DIR%..\..
set APK_DIR=%PROJECT_DIR%\android\apk

echo Building Android APK...

cd /d "%APK_DIR%"

if exist "gradlew.bat" (
    call gradlew.bat assembleRelease
) else (
    echo ERROR: gradlew.bat not found
    echo Please ensure you have run 'gradle wrapper' first or have Gradle installed
    exit /b 1
)

if %ERRORLEVEL% neq 0 (
    echo Build failed
    exit /b 1
)

echo.
echo Build successful!
echo Output: %APK_DIR%\app\build\outputs\apk\release\app-release.apk

endlocal