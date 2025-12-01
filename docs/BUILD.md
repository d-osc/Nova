# Nova Compiler - Build Instructions

## Prerequisites

### Windows

1. **LLVM 16+**
   ```powershell
   # Download from https://github.com/llvm/llvm-project/releases
   # Or use Chocolatey:
   choco install llvm --version=16.0.0
   ```

2. **CMake 3.20+**
   ```powershell
   choco install cmake
   ```

3. **Visual Studio 2022**
   - Install "Desktop development with C++" workload
   - Or use Build Tools for Visual Studio 2022

4. **Node.js 18+** (optional)
   ```powershell
   choco install nodejs
   ```

### Linux (Ubuntu/Debian)

```bash
# LLVM 16
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 16

# CMake
sudo apt-get install cmake

# Build essentials
sudo apt-get install build-essential

# Node.js (optional)
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs
```

### macOS

```bash
# Using Homebrew
brew install llvm@16
brew install cmake
brew install node  # optional

# Add LLVM to PATH
export PATH="/usr/local/opt/llvm@16/bin:$PATH"
```

## Building

### Quick Build

```bash
# Clone repository
git clone https://github.com/nova-lang/compiler
cd compiler

# Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release -j$(nproc)

# Install
cmake --install build
```

### Platform-Specific Builds

#### Windows (Visual Studio)

```powershell
# Set LLVM directory
$env:LLVM_DIR = "C:\Program Files\LLVM\lib\cmake\llvm"

# Configure for Visual Studio 2022
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_BUILD_TYPE=Release `
    -DLLVM_DIR="$env:LLVM_DIR"

# Build
cmake --build build --config Release --parallel

# Install
cmake --install build --prefix "C:\Program Files\Nova"
```

#### Linux

```bash
# Configure
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DLLVM_DIR=/usr/lib/llvm-16/cmake

# Build
cmake --build build --parallel $(nproc)

# Install (requires sudo)
sudo cmake --install build
```

#### macOS

```bash
# Configure
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DLLVM_DIR=/usr/local/opt/llvm@16/lib/cmake/llvm

# Build
cmake --build build --parallel $(sysctl -n hw.ncpu)

# Install
sudo cmake --install build
```

## Build Options

```bash
# Debug build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

# With tests
cmake -B build -S . -DBUILD_TESTING=ON

# With documentation
cmake -B build -S . -DBUILD_DOCS=ON

# Without examples
cmake -B build -S . -DBUILD_EXAMPLES=OFF

# Custom LLVM location
cmake -B build -S . -DLLVM_DIR=/path/to/llvm/cmake

# Custom installation prefix
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/custom/path
```

## Testing

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test
ctest --test-dir build -R lexer

# Run with verbose output
ctest --test-dir build -V

# Run in parallel
ctest --test-dir build -j$(nproc)
```

## Running Examples

```bash
# After building
cd build

# Run hello example
./nova ../examples/hello.ts

# Compile to executable
./nova compile ../examples/fibonacci.ts -O3 -o fib.exe

# Run the compiled executable
./fib.exe
```

## Development Build

For faster iteration during development:

```bash
# Use Ninja for faster builds
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Or just:
ninja -C build

# Rebuild specific target
ninja -C build nova
```

## Troubleshooting

### LLVM not found

```bash
# Windows
$env:LLVM_DIR = "C:\Program Files\LLVM\lib\cmake\llvm"

# Linux/macOS
export LLVM_DIR=/usr/lib/llvm-16/cmake
```

### CMake version too old

```bash
# Ubuntu
sudo apt-get install cmake

# Or build from source
wget https://github.com/Kitware/CMake/releases/download/v3.27.0/cmake-3.27.0.tar.gz
tar -xzf cmake-3.27.0.tar.gz
cd cmake-3.27.0
./bootstrap && make && sudo make install
```

### Compiler errors

Make sure you have a C++20-capable compiler:
- GCC 11+
- Clang 14+
- MSVC 2022+

### Linking errors

Make sure LLVM libraries are in your library path:

```bash
# Linux
export LD_LIBRARY_PATH=/usr/lib/llvm-16/lib:$LD_LIBRARY_PATH

# macOS
export DYLD_LIBRARY_PATH=/usr/local/opt/llvm@16/lib:$DYLD_LIBRARY_PATH
```

## IDE Setup

### Visual Studio Code

Install recommended extensions:
- C/C++ (Microsoft)
- CMake Tools
- CMake

```json
// .vscode/settings.json
{
  "cmake.configureOnOpen": true,
  "cmake.buildDirectory": "${workspaceFolder}/build",
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"
}
```

### CLion

CLion has built-in CMake support. Just open the project folder.

### Visual Studio 2022

Open the folder, Visual Studio will automatically detect CMakeLists.txt.

## Clean Build

```bash
# Remove build directory
rm -rf build

# Or use CMake
cmake --build build --target clean

# Then rebuild
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Contributing

See [CONTRIBUTING.md](../CONTRIBUTING.md) for development guidelines.
