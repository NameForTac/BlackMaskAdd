@echo off
setlocal EnableDelayedExpansion

REM ==========================================================
REM  Qt MinGW Build Script (Safe Mode: Uses Temp Folder)
REM ==========================================================

REM 1. Set Paths
set "Qt_ROOT=D:\QT\6.10.2\mingw_64"
set "MinGW_TOOLCHAIN=D:\QT\Tools\mingw1310_64"
set "Ninja_TOOL=D:\QT\Tools\Ninja"
set "CMake_TOOL=D:\QT\Tools\CMake_64"

REM Verify paths
if not exist "%Qt_ROOT%\bin\qmake.exe" (
    echo [ERROR] Qt qmake not found at: %Qt_ROOT%\bin\qmake.exe
    pause
    exit /b 1
)

set "PATH=%MinGW_TOOLCHAIN%\bin;%Ninja_TOOL%;%CMake_TOOL%\bin;%Qt_ROOT%\bin;%PATH%"

REM ==========================================================
REM  COPY TO TEMP FOLDER (Bypass Chinese Path Issues)
REM ==========================================================
set "ORIGINAL_DIR=%~dp0"
set "SAFE_BUILD_DIR=C:\QtBuildTemp\ImageMaskTool"

echo [INFO] Copying project to safe path: %SAFE_BUILD_DIR%
if exist "%SAFE_BUILD_DIR%" (
    rmdir /s /q "%SAFE_BUILD_DIR%"
)
mkdir "%SAFE_BUILD_DIR%"

REM Copy source files only (skip existing build folders to be safe)
xcopy /s /y /i "%ORIGINAL_DIR%src" "%SAFE_BUILD_DIR%\src" >nul
xcopy /s /y /i "%ORIGINAL_DIR%include" "%SAFE_BUILD_DIR%\include" >nul
copy /y "%ORIGINAL_DIR%CMakeLists.txt" "%SAFE_BUILD_DIR%\" >nul

cd /d "%SAFE_BUILD_DIR%"
mkdir build
cd build

REM ==========================================================
REM  BUILD
REM ==========================================================
echo [INFO] Configuring CMake...
cmake -G "Ninja" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%Qt_ROOT%" ^
    -DCMAKE_MAKE_PROGRAM="%Ninja_TOOL%\ninja.exe" ^
    -DCMAKE_C_COMPILER="%MinGW_TOOLCHAIN%\bin\gcc.exe" ^
    -DCMAKE_CXX_COMPILER="%MinGW_TOOLCHAIN%\bin\g++.exe" ^
    ..

if %errorlevel% neq 0 (
    echo [ERROR] CMake Configuration Failed!
    pause
    exit /b 1
)

echo [INFO] Building...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Build Failed!
    pause
    exit /b 1
)

REM ==========================================================
REM  COPY OUTPUT BACK (Packaged Folder)
REM ==========================================================
set "RELEASE_DIR=%ORIGINAL_DIR%Release"
echo [INFO] Creating Release folder at: %RELEASE_DIR%
if not exist "%RELEASE_DIR%" mkdir "%RELEASE_DIR%"

echo [INFO] Copying executable...
copy /y "ImageMaskTool.exe" "%RELEASE_DIR%\ImageMaskTool.exe"

echo [INFO] Deploying Qt DLLs and Plugins (windeployqt)...
REM Use windeployqt to copy all dependencies into the Release folder
REM Ensure imageformats are included (especially for TGA/JPG)
"%Qt_ROOT%\bin\windeployqt.exe" --release --compiler-runtime --no-translations "%RELEASE_DIR%\ImageMaskTool.exe"

if exist "%RELEASE_DIR%\ImageMaskTool.exe" (
    echo [SUCCESS] Build Complete!
    echo [INFO] Output is in: %RELEASE_DIR%
    
    echo [INFO] Creating qt.conf to force plugin loading...
    echo [Paths] > "%RELEASE_DIR%\qt.conf"
    echo Plugins=. >> "%RELEASE_DIR%\qt.conf"
    
    echo [INFO] Forcing manual copy of ALL image plugins...
    if not exist "%RELEASE_DIR%\imageformats" mkdir "%RELEASE_DIR%\imageformats"
    REM Copy *all* dlls just in case
    copy /y "%Qt_ROOT%\plugins\imageformats\*.dll" "%RELEASE_DIR%\imageformats\" >nul
    
    echo [INFO] Listing copied plugins:
    dir "%RELEASE_DIR%\imageformats\*.dll" | findstr "dll"

    if not exist "%RELEASE_DIR%\imageformats\qtga.dll" (
        echo [ERROR] CRITICAL: qtga.dll WAS NOT COPIED! Check %Qt_ROOT%\plugins\imageformats path!
    ) else (
        echo [SUCCESS] qtga.dll verified.
    )
    
    echo [INFO] Starting Application...
    start "" "%RELEASE_DIR%\ImageMaskTool.exe"
) else (
    echo [ERROR] Failed to output executable!
)

REM Cleanup (Optional - uncomment to keep temp files for debugging)
REM cd /d "%ORIGINAL_DIR%"
REM rmdir /s /q "%SAFE_BUILD_DIR%"

pause
exit /b 0
