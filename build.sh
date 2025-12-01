#!/bin/bash
# Nova Compiler Build Script for Linux/macOS
# Requires: CMake 3.20+, LLVM 16+, GCC 11+ or Clang 14+

set -e

echo ""
echo "╔════════════════════════════════════════════════════════╗"
echo "║            Nova Compiler - Build Script               ║"
echo "║                  Linux/macOS Build                     ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macOS"
    DEFAULT_LLVM_DIR="/usr/local/opt/llvm@16/lib/cmake/llvm"
else
    OS="Linux"
    DEFAULT_LLVM_DIR="/usr/lib/llvm-16/cmake"
fi

echo "[INFO] Operating System: $OS"

# Check if LLVM_DIR is set
if [ -z "$LLVM_DIR" ]; then
    echo "[INFO] LLVM_DIR not set, using default location..."
    LLVM_DIR="$DEFAULT_LLVM_DIR"
fi

echo "[INFO] LLVM Directory: $LLVM_DIR"

# Check if CMake is available
if ! command -v cmake &> /dev/null; then
    echo "[ERROR] CMake not found!"
    echo "[ERROR] Please install CMake 3.20 or later"
    exit 1
fi

echo "[INFO] CMake found:"
cmake --version | head -n 1

# Check if LLVM directory exists
if [ ! -d "$LLVM_DIR" ]; then
    echo "[WARNING] LLVM directory not found: $LLVM_DIR"
    echo "[WARNING] Build may fail if LLVM is not properly installed"
fi

# Clean previous build if requested
if [ "$1" == "clean" ]; then
    echo ""
    echo "[CLEAN] Removing build directory..."
    rm -rf build
    echo "[CLEAN] Done!"
    if [ -z "$2" ]; then
        exit 0
    fi
fi

# Create build directory
mkdir -p build

echo ""
echo "═══════════════════════════════════════════════════════"
echo "Phase 1: CMake Configuration"
echo "═══════════════════════════════════════════════════════"
echo ""

# Detect number of CPU cores
if [[ "$OS" == "macOS" ]]; then
    NCORES=$(sysctl -n hw.ncpu)
else
    NCORES=$(nproc)
fi

echo "[INFO] Using $NCORES parallel jobs"

# Configure with CMake
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DLLVM_DIR="$LLVM_DIR"

if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] CMake configuration failed!"
    echo "[ERROR] Please check LLVM installation and LLVM_DIR variable"
    exit 1
fi

echo ""
echo "[SUCCESS] Configuration completed!"
echo ""
echo "═══════════════════════════════════════════════════════"
echo "Phase 2: Building Nova Compiler"
echo "═══════════════════════════════════════════════════════"
echo ""

# Build
cmake --build build --parallel $NCORES

if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] Build failed!"
    exit 1
fi

echo ""
echo "╔════════════════════════════════════════════════════════╗"
echo "║            ✅ Build Completed Successfully!             ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""
echo "[INFO] Executable location: build/nova"
echo ""
echo "[USAGE] To run the compiler:"
echo "        ./build/nova --help"
echo "        ./build/nova compile examples/hello.ts"
echo ""

# Run tests if requested
if [ "$1" == "test" ]; then
    echo "═══════════════════════════════════════════════════════"
    echo "Running Tests"
    echo "═══════════════════════════════════════════════════════"
    echo ""
    cd build
    ctest --output-on-failure --parallel $NCORES
    cd ..
fi

# Install if requested
if [ "$1" == "install" ]; then
    echo "═══════════════════════════════════════════════════════"
    echo "Installing Nova Compiler"
    echo "═══════════════════════════════════════════════════════"
    echo ""
    sudo cmake --install build
    echo ""
    echo "[SUCCESS] Installation completed!"
    echo "[INFO] Nova compiler installed to: /usr/local/bin/nova"
fi

exit 0
