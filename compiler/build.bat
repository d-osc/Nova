@echo off
REM Nova Compiler Build Script for Windows
REM Requires: Visual Studio 2022, CMake, LLVM 16+

echo.
echo ╔════════════════════════════════════════════════════════╗
echo ║            Nova Compiler - Build Script               ║
echo ║                    Windows Build                       ║
echo ╚════════════════════════════════════════════════════════╝
echo.

REM Check if LLVM_DIR is set
if "%LLVM_DIR%"=="" (
    echo [INFO] LLVM_DIR not set, using default location...
    set LLVM_DIR=C:\Program Files\LLVM\lib\cmake\llvm
)

echo [INFO] LLVM Directory: %LLVM_DIR%

REM Check if CMake is available
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake not found in PATH!
    echo [ERROR] Please install CMake or add it to PATH
    exit /b 1
)

echo [INFO] CMake found: 
cmake --version | findstr /R "version"

REM Clean previous build (optional)
if "%1"=="clean" (
    echo.
    echo [CLEAN] Removing build directory...
    if exist build rd /s /q build
    echo [CLEAN] Done!
    if "%2"=="" exit /b 0
)

REM Create build directory
if not exist build mkdir build

echo.
echo ═══════════════════════════════════════════════════════
echo Phase 1: CMake Configuration
echo ═══════════════════════════════════════════════════════
echo.

REM Configure with CMake
cmake -B build -S . ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DLLVM_DIR="%LLVM_DIR%"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    echo [ERROR] Please check LLVM installation and LLVM_DIR variable
    exit /b 1
)

echo.
echo [SUCCESS] Configuration completed!
echo.
echo ═══════════════════════════════════════════════════════
echo Phase 2: Building Nova Compiler
echo ═══════════════════════════════════════════════════════
echo.

REM Build
cmake --build build --config Release --parallel

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed!
    exit /b 1
)

echo.
echo ╔════════════════════════════════════════════════════════╗
echo ║            ✅ Build Completed Successfully!             ║
echo ╚════════════════════════════════════════════════════════╝
echo.
echo [INFO] Executable location: build\Release\nova.exe
echo.
echo [USAGE] To run the compiler:
echo         build\Release\nova.exe --help
echo         build\Release\nova.exe compile examples\hello.ts
echo.

REM Run tests if requested
if "%1"=="test" (
    echo ═══════════════════════════════════════════════════════
    echo Running Tests
    echo ═══════════════════════════════════════════════════════
    echo.
    cd build
    ctest --output-on-failure --build-config Release
    cd ..
)

exit /b 0
