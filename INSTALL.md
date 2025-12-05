# Nova Language - Installation Guide

This guide will help you install Nova on your system.

## Quick Install

Nova provides automated installation scripts for all major operating systems.

### Universal Installer (Linux/macOS)

```bash
./install.sh
```

The universal installer automatically detects your OS and runs the appropriate installer.

---

## Platform-Specific Installation

### 🐧 Linux

#### Automatic Installation

```bash
./install-linux.sh
```

**Supported Distributions:**
- Ubuntu 18.04+
- Debian 10+
- Fedora 30+
- CentOS 8+
- Arch Linux
- Other distributions (may require manual dependency installation)

**What it does:**
1. Detects your Linux distribution
2. Installs build tools (GCC/Clang, CMake, Ninja)
3. Installs LLVM 18
4. Builds Nova from source
5. Installs Nova to `/usr/local/bin`

**Requirements:**
- Root/sudo access (for installing dependencies)
- ~2GB disk space
- ~15 minutes installation time

#### Manual Installation (Linux)

If the automatic installer doesn't work for your distribution:

```bash
# 1. Install dependencies
# Ubuntu/Debian:
sudo apt-get install build-essential cmake ninja-build git libssl-dev zlib1g-dev

# Fedora/CentOS:
sudo dnf install gcc gcc-c++ cmake ninja-build git openssl-devel zlib-devel

# Arch Linux:
sudo pacman -S base-devel cmake ninja git openssl zlib

# 2. Install LLVM 18
# Follow instructions at: https://llvm.org/

# 3. Build Nova
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 4. Install
sudo cmake --install build
```

---

### 🍎 macOS

#### Automatic Installation

```bash
./install-macos.sh
```

**Supported Versions:**
- macOS 10.15 (Catalina) or later
- Works on both Intel and Apple Silicon (M1/M2/M3)

**What it does:**
1. Installs Homebrew (if not present)
2. Installs Xcode Command Line Tools (if not present)
3. Installs dependencies (CMake, Ninja, etc.)
4. Installs LLVM 18
5. Builds Nova from source
6. Installs Nova to `/usr/local/bin`

**Requirements:**
- Administrator access
- ~2GB disk space
- ~20 minutes installation time (first-time Homebrew setup may take longer)

#### Manual Installation (macOS)

```bash
# 1. Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 2. Install dependencies
brew install cmake ninja llvm@18

# 3. Add LLVM to PATH (add to ~/.zshrc or ~/.bash_profile)
export PATH="$(brew --prefix llvm@18)/bin:$PATH"

# 4. Build Nova
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 5. Install
sudo cmake --install build
```

---

### 🪟 Windows

#### Automatic Installation

**Run PowerShell as Administrator**, then:

```powershell
.\install-windows.ps1
```

**Supported Versions:**
- Windows 10 (version 1903 or later)
- Windows 11

**What it does:**
1. Installs Chocolatey package manager (if not present)
2. Installs Visual Studio Build Tools
3. Installs dependencies (CMake, Ninja, Git, Python)
4. Installs LLVM 18
5. Builds Nova from source
6. Installs Nova to `C:\Program Files\Nova`
7. Adds Nova to system PATH

**Requirements:**
- Administrator privileges
- ~5GB disk space (includes Visual Studio Build Tools)
- ~30 minutes installation time

#### Manual Installation (Windows)

```powershell
# 1. Install Visual Studio 2019/2022 with C++ development tools
# Download from: https://visualstudio.microsoft.com/

# 2. Install CMake and Ninja
# Download from: https://cmake.org/ and https://ninja-build.org/

# 3. Install LLVM 18
# Download from: https://github.com/llvm/llvm-project/releases

# 4. Build Nova
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR="C:\Program Files\LLVM\lib\cmake\llvm"
cmake --build build --config Release

# 5. Add to PATH manually or copy build\Release\nova.exe to a directory in PATH
```

---

## Verification

After installation, verify Nova is working:

```bash
# Check version
nova --version

# Show help
nova --help

# Run a simple test
echo "console.log('Hello from Nova!');" > test.ts
nova run test.ts
```

You should see:
```
Nova 1.0.0
The Fastest JavaScript Runtime

Hello from Nova!
```

---

## Quick Start

### Running TypeScript/JavaScript

```bash
# Run directly
nova run hello.ts

# Run with debugging
NOVA_DEBUG=1 nova run app.ts
```

### Building Executables

```bash
# Build to native executable
nova build app.ts

# Build with optimizations
nova build app.ts -O3

# Build with custom output name
nova build app.ts -o myapp
```

### Package Management

```bash
# Install packages
nova install express

# Install dev dependencies
nova install --save-dev typescript

# Update packages
nova update

# Remove package
nova uninstall lodash
```

---

## Troubleshooting

### "nova: command not found"

**Solution:** Restart your terminal/shell to reload PATH, or manually add Nova to PATH:

**Linux/macOS:**
```bash
export PATH="/usr/local/bin:$PATH"
```

**Windows:**
```powershell
$env:Path += ";C:\Program Files\Nova"
```

### LLVM not found during build

**Solution:** Specify LLVM path explicitly:

```bash
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH="/path/to/llvm"
```

### Build fails on Linux with "cannot find -lstdc++"

**Solution:** Install libstdc++:
```bash
# Ubuntu/Debian
sudo apt-get install libstdc++-10-dev

# Fedora
sudo dnf install libstdc++-devel
```

### Windows: "Visual Studio not found"

**Solution:** Install Visual Studio Build Tools:
```powershell
choco install visualstudio2019buildtools --package-parameters "--add Microsoft.VisualStudio.Workload.VCTools"
```

### macOS: "xcode-select: error: tool 'xcodebuild' requires Xcode"

**Solution:** Install Xcode Command Line Tools:
```bash
xcode-select --install
```

---

## Uninstallation

### Linux/macOS

```bash
sudo rm /usr/local/bin/nova
sudo rm -rf /usr/local/lib/nova
```

### Windows

```powershell
Remove-Item "C:\Program Files\Nova" -Recurse -Force
# Then remove from PATH in System Environment Variables
```

---

## Advanced Options

### Custom Installation Directory

**Linux/macOS:**
```bash
INSTALL_DIR=/opt/nova ./install-linux.sh
```

**Windows:**
```powershell
.\install-windows.ps1 -InstallDir "D:\Nova"
```

### Skip LLVM Installation

If you already have LLVM 18 installed:

**Linux/macOS:**
```bash
# Edit the script and comment out the install_llvm function call
```

**Windows:**
```powershell
.\install-windows.ps1 -SkipLLVM
```

### Build Only (No Install)

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Binary will be in: build/nova (Linux/macOS) or build\Release\nova.exe (Windows)
```

---

## System Requirements

### Minimum Requirements
- **OS:** Linux (kernel 4.x+), macOS 10.15+, Windows 10 (1903+)
- **RAM:** 4GB (8GB recommended)
- **Disk:** 2GB free space
- **CPU:** x64 processor with SSE4.2 support

### Recommended Requirements
- **RAM:** 16GB
- **Disk:** 10GB free space (for building from source)
- **CPU:** Multi-core x64 processor with AVX2 support

---

## Docker Installation

For containerized environments:

```bash
# Build Docker image
docker build -t nova-lang .

# Run Nova in Docker
docker run -it nova-lang nova --version
```

---

## Getting Help

- **Documentation:** https://nova-lang.org/docs
- **GitHub Issues:** https://github.com/d-osc/nova-lang/issues
- **Discord:** https://discord.gg/nova-lang
- **Stack Overflow:** Tag your questions with `nova-lang`

---

## Next Steps

After installation:

1. Read the [Getting Started Guide](https://nova-lang.org/docs/getting-started)
2. Check out [Examples](./examples/)
3. Run the [Benchmarks](./benchmarks/)
4. Join our [Community](https://discord.gg/nova-lang)

---

**Enjoy using Nova - The Fastest JavaScript Runtime!** 🚀
