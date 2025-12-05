# Nova Multi-Platform Docker Build Guide

Complete guide for building Nova Programming Language for all supported platforms using Docker.

## Table of Contents

- [Overview](#overview)
- [Supported Platforms](#supported-platforms)
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Build Commands](#build-commands)
- [Platform-Specific Builds](#platform-specific-builds)
- [Development Environment](#development-environment)
- [Testing](#testing)
- [Output Structure](#output-structure)
- [Troubleshooting](#troubleshooting)
- [Advanced Usage](#advanced-usage)
- [CI/CD Integration](#cicd-integration)
- [macOS Build Requirements](#macos-build-requirements)

---

## Overview

The Nova Docker build system provides a complete cross-compilation environment for building Nova binaries across multiple operating systems and architectures from a single Linux host.

### Key Features

- **Multi-platform support**: Build for 5 different platforms from one system
- **Reproducible builds**: Consistent build environment using Docker containers
- **Optimized binaries**: Full optimization flags (-O3, SIMD, LTO)
- **Parallel builds**: Ninja build system for maximum performance
- **Build caching**: Docker volumes for incremental builds
- **Interactive development**: Full development environment included

### Architecture

```
Host System (Any OS with Docker)
    ↓
Docker Build Environment (Ubuntu 22.04 + LLVM 16)
    ↓
Multi-Stage Build Process
    ├─→ Linux x86-64 (native Clang-16)
    ├─→ Linux ARM64 (cross-compile with aarch64-gcc)
    ├─→ Windows x86-64 (cross-compile with MinGW)
    ├─→ macOS x86-64 (cross-compile with osxcross)
    └─→ macOS ARM64 (cross-compile with osxcross)
    ↓
Output: build/docker-output/[platform]/nova[.exe]
```

---

## Supported Platforms

| Platform | Architecture | Compiler | Status | Output |
|----------|-------------|----------|--------|--------|
| **Linux** | x86-64 | Clang-16 | ✅ Stable | `nova` |
| **Linux** | ARM64 | aarch64-gcc | ✅ Stable | `nova` |
| **Windows** | x86-64 | MinGW-w64 | ✅ Stable | `nova.exe` |
| **macOS** | x86-64 (Intel) | osxcross | ⚠️ Requires SDK | `nova` |
| **macOS** | ARM64 (Apple Silicon) | osxcross | ⚠️ Requires SDK | `nova` |

### Optimization Flags by Platform

**Linux x86-64**:
```
-O3 -march=x86-64-v3 -mavx2 -DNDEBUG
```
- AVX2 SIMD instructions
- x86-64-v3 microarchitecture level
- Full optimization

**Linux ARM64**:
```
-O3 -march=armv8-a+simd -DNDEBUG
```
- ARMv8 NEON SIMD instructions
- Full optimization

**Windows x86-64**:
```
-O3 -march=x86-64 -DNDEBUG -static-libgcc -static-libstdc++
```
- Static linking for portability
- x86-64 baseline compatibility

**macOS (both)**:
```
-O3 -march=[x86-64|armv8-a] -DNDEBUG
```
- Platform-specific optimization
- Requires macOS SDK

---

## Prerequisites

### Required Software

1. **Docker**: Version 20.10 or higher
   ```bash
   # Check Docker version
   docker --version
   ```

2. **Docker Compose**: Version 2.0 or higher (or `docker-compose` v1.29+)
   ```bash
   # Check Docker Compose version
   docker compose version
   # OR
   docker-compose --version
   ```

### System Requirements

- **Disk Space**: Minimum 10 GB free (20 GB recommended for all platforms)
- **RAM**: Minimum 4 GB (8 GB recommended for parallel builds)
- **CPU**: Multi-core recommended for faster builds
- **Network**: Internet connection for downloading dependencies

### Installation

**Linux**:
```bash
# Docker
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER

# Docker Compose (if not included)
sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose
```

**macOS**:
```bash
# Install Docker Desktop for Mac
# Download from: https://www.docker.com/products/docker-desktop
```

**Windows**:
```powershell
# Install Docker Desktop for Windows
# Download from: https://www.docker.com/products/docker-desktop
# Enable WSL 2 backend
```

---

## Quick Start

### Build All Platforms

Build Nova for all supported platforms:

```bash
# Make build script executable (first time only)
chmod +x docker-build.sh

# Build all platforms
./docker-build.sh all
```

This will:
1. Build Docker images for all platforms
2. Compile Nova for each platform
3. Copy binaries to `build/docker-output/`
4. Display build summary

Expected output:
```
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║        Nova Programming Language - Multi-Platform Build     ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝

[Nova Build] Building for ALL platforms...
[Nova Build] ✓ All platforms built successfully
[Nova Build] Binaries copied to: build/docker-output/

Built binaries:
-rwxr-xr-x 1 user user 15M build/docker-output/linux-x64/nova
-rwxr-xr-x 1 user user 16M build/docker-output/linux-arm64/nova
-rwxr-xr-x 1 user user 18M build/docker-output/windows-x64/nova.exe
```

### Verify Binaries

```bash
# Linux x86-64 (if on Linux)
./build/docker-output/linux-x64/nova --version

# Windows (if on Windows or Wine)
./build/docker-output/windows-x64/nova.exe --version
```

---

## Build Commands

### Help / Usage

Display all available commands:

```bash
./docker-build.sh help
```

Output:
```
Usage: ./docker-build.sh [PLATFORM]

Platforms:
  all            Build for all platforms (default)
  linux-x64      Build for Linux x86-64
  linux-arm64    Build for Linux ARM64
  windows-x64    Build for Windows x86-64
  macos-x64      Build for macOS x86-64 (requires SDK)
  macos-arm64    Build for macOS ARM64 (requires SDK)

Development:
  dev            Start interactive development environment
  test           Run test suite
  clean          Remove all build artifacts

Examples:
  ./docker-build.sh                    # Build all platforms
  ./docker-build.sh linux-x64          # Build only Linux x86-64
  ./docker-build.sh dev                # Start dev environment
```

### Clean Build Artifacts

Remove all build outputs and Docker images:

```bash
./docker-build.sh clean
```

This removes:
- `build/docker-output/` directory
- All `nova-lang:*` Docker images
- Docker build cache (optional)

---

## Platform-Specific Builds

### Linux x86-64

Build only for Linux x86-64:

```bash
./docker-build.sh linux-x64
```

**Specifications**:
- Compiler: Clang-16
- Architecture: x86-64-v3 (AVX2 support)
- Optimization: `-O3 -mavx2`
- Output: `build/docker-output/linux-x64/nova`

**Testing**:
```bash
# Run on any Linux x86-64 system
./build/docker-output/linux-x64/nova --version
./build/docker-output/linux-x64/nova run examples/hello.ts
```

### Linux ARM64

Build for Linux ARM64 (Raspberry Pi, AWS Graviton, etc.):

```bash
./docker-build.sh linux-arm64
```

**Specifications**:
- Compiler: aarch64-linux-gnu-gcc
- Architecture: ARMv8-A (NEON SIMD)
- Optimization: `-O3 -march=armv8-a+simd`
- Output: `build/docker-output/linux-arm64/nova`

**Testing**:
```bash
# Test with QEMU (on x86-64 host)
sudo apt-get install qemu-user-static
qemu-aarch64-static build/docker-output/linux-arm64/nova --version

# Or copy to ARM64 device
scp build/docker-output/linux-arm64/nova pi@raspberry:~/
ssh pi@raspberry './nova --version'
```

### Windows x86-64

Build for Windows:

```bash
./docker-build.sh windows-x64
```

**Specifications**:
- Compiler: x86_64-w64-mingw32-gcc (MinGW)
- Architecture: x86-64 baseline
- Optimization: `-O3 -march=x86-64`
- Linking: Static libgcc/libstdc++ (portable)
- Output: `build/docker-output/windows-x64/nova.exe`

**Testing**:
```bash
# Test with Wine (on Linux host)
sudo apt-get install wine64
wine64 build/docker-output/windows-x64/nova.exe --version

# Or copy to Windows machine
# Then run: nova.exe --version
```

### macOS x86-64 (Intel)

Build for macOS Intel:

```bash
./docker-build.sh macos-x64
```

**⚠️ Requirements**: macOS SDK required (see [macOS Build Requirements](#macos-build-requirements))

**Specifications**:
- Compiler: osxcross (o64-clang++)
- Architecture: x86-64
- Optimization: `-O3 -march=x86-64`
- Output: `build/docker-output/macos-x64/nova`

### macOS ARM64 (Apple Silicon)

Build for Apple Silicon:

```bash
./docker-build.sh macos-arm64
```

**⚠️ Requirements**: macOS SDK required (see [macOS Build Requirements](#macos-build-requirements))

**Specifications**:
- Compiler: osxcross (oa64-clang++)
- Architecture: ARM64
- Optimization: `-O3 -march=armv8-a`
- Output: `build/docker-output/macos-arm64/nova`

---

## Development Environment

### Starting Interactive Shell

Launch an interactive development environment:

```bash
./docker-build.sh dev
```

This starts a Docker container with:
- Full Nova source code mounted at `/nova`
- All build tools (CMake, Ninja, LLVM 16)
- Cross-compilation toolchains
- Build cache volume for incremental builds

### Development Workflow

Inside the development environment:

```bash
# You're now in the container at /nova
cd /nova

# Configure build
mkdir -p build/dev
cd build/dev
cmake ../.. -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build
ninja nova

# Run
./nova --version
./nova run ../../examples/hello.ts

# Edit files
# (Changes persist on host filesystem)

# Exit
exit
```

### Build Cache

The development environment uses a Docker volume (`nova-build-cache`) to cache build artifacts:

- **First build**: ~5-10 minutes
- **Incremental builds**: ~30 seconds - 2 minutes

To clear cache:
```bash
docker volume rm nova-build-cache
```

---

## Testing

### Run Test Suite

Execute Nova's test suite in Docker:

```bash
./docker-build.sh test
```

This runs:
1. Unit tests (if configured)
2. Integration tests
3. Language feature tests

Expected output:
```
[Nova Build] Running tests...
Test project /nova/build/linux-x64
    Start 1: ParserTests
1/5 Test #1: ParserTests .......................   Passed    0.42 sec
    Start 2: CodeGenTests
2/5 Test #2: CodeGenTests ......................   Passed    1.23 sec
...
100% tests passed, 0 tests failed out of 5
```

### Manual Testing

Test specific functionality:

```bash
# Start dev environment
./docker-build.sh dev

# Inside container
cd /nova/build/dev
./nova run ../../tests/features/arrays.ts
./nova run ../../tests/features/loops.ts
./nova run ../../tests/features/functions.ts
```

---

## Output Structure

After building, outputs are organized by platform:

```
build/docker-output/
├── BUILD_INFO.txt                # Build metadata
├── README.md                     # Nova documentation
├── LICENSE                       # License file
├── linux-x64/
│   └── nova                      # Linux x86-64 binary (15-20 MB)
├── linux-arm64/
│   └── nova                      # Linux ARM64 binary (16-22 MB)
├── windows-x64/
│   └── nova.exe                  # Windows binary (18-25 MB)
├── macos-x64/
│   └── nova                      # macOS Intel binary
└── macos-arm64/
    └── nova                      # macOS Apple Silicon binary
```

### Binary Sizes

Typical sizes (Release build, stripped):
- **Linux x86-64**: ~15 MB
- **Linux ARM64**: ~16 MB
- **Windows**: ~18 MB (static linking adds overhead)
- **macOS**: ~14-16 MB

### Build Info

The `BUILD_INFO.txt` file contains:
```
Nova Programming Language - Multi-Platform Build
Build Date: 2025-12-04 10:30:45 UTC
Supported Platforms:
  - Linux x86-64
  - Linux ARM64
  - Windows x86-64
  - macOS x86-64
  - macOS ARM64 (Apple Silicon)
```

---

## Troubleshooting

### Common Issues

#### 1. Docker Not Running

**Error**:
```
Cannot connect to the Docker daemon
```

**Solution**:
```bash
# Linux
sudo systemctl start docker

# macOS/Windows
# Start Docker Desktop application
```

#### 2. Permission Denied

**Error**:
```
permission denied while trying to connect to the Docker daemon socket
```

**Solution**:
```bash
# Add user to docker group (Linux)
sudo usermod -aG docker $USER
newgrp docker
```

#### 3. Disk Space Full

**Error**:
```
no space left on device
```

**Solution**:
```bash
# Clean up Docker
docker system prune -a
docker volume prune

# Free up space
./docker-build.sh clean
```

#### 4. Build Fails with "ninja not found"

**Error**:
```
CMake Error: CMake was unable to find a build program corresponding to "Ninja"
```

**Solution**:
This should not happen with the provided Dockerfile. If it does:
```bash
# Rebuild Docker image from scratch
docker-compose build --no-cache build-linux-x64
```

#### 5. LLVM Not Found

**Error**:
```
Could NOT find LLVM
```

**Solution**:
Rebuild base Docker image:
```bash
docker-compose build --no-cache base
```

#### 6. Windows Build Hangs

**Symptom**: MinGW build appears to hang

**Solution**:
- This is normal for first build (linking takes time)
- Wait 5-10 minutes
- If truly stuck, check Docker logs:
  ```bash
  docker logs nova-build-windows-x64
  ```

#### 7. ARM64 Build "Illegal instruction"

**Error** (when running ARM64 binary on x86-64):
```
Illegal instruction (core dumped)
```

**Solution**:
ARM64 binaries require ARM64 CPU or QEMU:
```bash
# Install QEMU
sudo apt-get install qemu-user-static

# Run with QEMU
qemu-aarch64-static build/docker-output/linux-arm64/nova --version
```

### Debugging

#### Enable Verbose Output

Edit `docker-build.sh` to add verbose flags:
```bash
# Add -v flag to docker-compose
$COMPOSE_CMD build -v ${platform}
```

#### Check Container Logs

```bash
# View logs from last build
docker logs nova-build-linux-x64

# Follow logs during build
docker logs -f nova-build-all
```

#### Interactive Debugging

Start development container and debug:
```bash
./docker-build.sh dev

# Inside container
cd /nova/build/linux-x64
cmake ../.. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=ON
ninja -v
```

---

## Advanced Usage

### Custom Build Flags

Modify `docker-compose.yml` to add custom environment variables:

```yaml
services:
  build-linux-x64:
    environment:
      - BUILD_TYPE=Release
      - ENABLE_OPTIMIZATIONS=ON
      - CUSTOM_CXX_FLAGS=-march=native -O3 -flto
```

### Build Specific Commit

Build from a specific Git commit:

```bash
# Checkout specific commit
git checkout abc123

# Build
./docker-build.sh all

# Return to master
git checkout master
```

### Parallel Platform Builds

Build multiple platforms simultaneously:

```bash
# Start builds in parallel (requires sufficient RAM)
docker-compose build build-linux-x64 &
docker-compose build build-windows-x64 &
docker-compose build build-linux-arm64 &
wait

echo "All builds complete!"
```

### Custom Output Directory

Modify volume mount in `docker-compose.yml`:

```yaml
volumes:
  - ./custom-output:/output  # Changed from ./build/docker-output
```

### LTO (Link-Time Optimization)

Enable LTO for smaller, faster binaries:

Edit `Dockerfile`, add to CMake flags:
```dockerfile
RUN cmake ../.. \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang-16 \
    -DCMAKE_CXX_COMPILER=clang++-16 \
    -DCMAKE_CXX_FLAGS="-O3 -march=x86-64-v3 -mavx2 -DNDEBUG -flto=thin" \
    -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -flto=thin" \
    && ninja nova
```

**Note**: LTO increases build time by 2-3x but reduces binary size by 10-20%.

---

## CI/CD Integration

### GitHub Actions

Create `.github/workflows/docker-build.yml`:

```yaml
name: Nova Multi-Platform Build

on:
  push:
    branches: [ master, develop ]
  pull_request:
    branches: [ master ]

jobs:
  build-all:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v3

    - name: Set up Docker Buildx
      uses: docker/setup-buildx-action@v2

    - name: Build all platforms
      run: |
        chmod +x docker-build.sh
        ./docker-build.sh all

    - name: Upload artifacts
      uses: actions/upload-artifact@v3
      with:
        name: nova-binaries
        path: build/docker-output/
        retention-days: 30

  test:
    runs-on: ubuntu-latest
    needs: build-all

    steps:
    - uses: actions/checkout@v3

    - name: Run tests
      run: |
        chmod +x docker-build.sh
        ./docker-build.sh test
```

### GitLab CI

Create `.gitlab-ci.yml`:

```yaml
image: docker:latest

services:
  - docker:dind

stages:
  - build
  - test

build-all:
  stage: build
  script:
    - apk add --no-cache bash
    - chmod +x docker-build.sh
    - ./docker-build.sh all
  artifacts:
    paths:
      - build/docker-output/
    expire_in: 30 days

test:
  stage: test
  script:
    - chmod +x docker-build.sh
    - ./docker-build.sh test
```

### Jenkins

Create `Jenkinsfile`:

```groovy
pipeline {
    agent any

    stages {
        stage('Build') {
            steps {
                sh 'chmod +x docker-build.sh'
                sh './docker-build.sh all'
            }
        }

        stage('Test') {
            steps {
                sh './docker-build.sh test'
            }
        }

        stage('Archive') {
            steps {
                archiveArtifacts artifacts: 'build/docker-output/**/*', fingerprint: true
            }
        }
    }

    post {
        always {
            cleanWs()
        }
    }
}
```

---

## macOS Build Requirements

### Overview

Building for macOS requires the **macOS SDK** which cannot be distributed with Nova due to licensing restrictions.

### Obtaining macOS SDK

#### Option 1: Extract from Xcode (Recommended)

On a macOS machine with Xcode installed:

```bash
# Locate SDK
ls /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/

# Create tarball
cd /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/
tar -czf MacOSX13.3.sdk.tar.gz MacOSX13.3.sdk/

# Copy to Nova project
scp MacOSX13.3.sdk.tar.gz user@linux-host:/path/to/Nova/
```

#### Option 2: Download Pre-Packaged SDK

Download from: https://github.com/joseluisq/macosx-sdks

```bash
cd /path/to/Nova
wget https://github.com/joseluisq/macosx-sdks/releases/download/13.3/MacOSX13.3.sdk.tar.xz
```

### Installing SDK in Docker

Modify `Dockerfile` to include SDK:

```dockerfile
# Install osxcross for macOS cross-compilation
WORKDIR /opt
RUN git clone https://github.com/tpoechtrager/osxcross.git && \
    cd osxcross

# Copy SDK (add this to your build process)
COPY MacOSX13.3.sdk.tar.gz /opt/osxcross/tarballs/

# Build osxcross
RUN cd /opt/osxcross && \
    UNATTENDED=1 ./build.sh

ENV PATH="/opt/osxcross/target/bin:${PATH}"
```

Then rebuild:
```bash
./docker-build.sh macos-x64
./docker-build.sh macos-arm64
```

### Testing macOS Binaries

macOS binaries must be tested on actual macOS systems:

```bash
# Copy to macOS machine
scp build/docker-output/macos-x64/nova user@mac:~/

# Test on macOS
ssh user@mac './nova --version'
```

### Troubleshooting macOS Builds

**Issue**: "macOS SDK not found"
- **Solution**: Ensure SDK is in `/opt/osxcross/tarballs/` before building osxcross

**Issue**: "ld: framework not found"
- **Solution**: Verify SDK includes all required frameworks

**Issue**: Binary segfaults on macOS
- **Solution**: Check SDK version matches target macOS version

---

## Performance Notes

### Build Times

Typical build times on modern hardware (8-core CPU, 16 GB RAM):

| Platform | First Build | Incremental Build |
|----------|------------|-------------------|
| Linux x86-64 | 3-5 min | 30-60 sec |
| Linux ARM64 | 4-6 min | 45-90 sec |
| Windows x86-64 | 5-8 min | 60-120 sec |
| macOS (both) | 6-10 min | 60-120 sec |
| **All platforms** | **15-25 min** | **3-5 min** |

### Optimization Levels

The Docker builds use **Release** mode with maximum optimization:

- **-O3**: Aggressive optimization
- **-march**: Architecture-specific instructions (AVX2, NEON)
- **-DNDEBUG**: Disable debug assertions
- **Strip**: Remove debug symbols

To build Debug version for development:
```bash
# Modify docker-compose.yml
environment:
  - BUILD_TYPE=Debug  # Changed from Release
```

Debug builds are 2-3x larger but include debugging symbols.

---

## Summary

### Quick Reference

**Build everything**:
```bash
./docker-build.sh all
```

**Build specific platform**:
```bash
./docker-build.sh linux-x64
./docker-build.sh windows-x64
```

**Development**:
```bash
./docker-build.sh dev
```

**Testing**:
```bash
./docker-build.sh test
```

**Clean up**:
```bash
./docker-build.sh clean
```

### Support

For issues or questions:
- **GitHub Issues**: https://github.com/yourusername/nova/issues
- **Documentation**: See project README.md
- **Build Logs**: Check `docker logs [container-name]`

---

## Appendix

### Docker Compose Reference

**Services**:
- `build-all` - Build all platforms
- `build-linux-x64` - Linux x86-64 only
- `build-linux-arm64` - Linux ARM64 only
- `build-windows-x64` - Windows x86-64 only
- `dev` - Interactive development environment
- `test` - Run test suite

**Volumes**:
- `nova-build-cache` - Persistent build cache (incremental builds)

### File Structure

```
Nova/
├── Dockerfile              # Multi-stage build definition
├── docker-compose.yml      # Service orchestration
├── docker-build.sh         # Build automation script
├── .dockerignore          # Exclude unnecessary files
├── DOCKER_BUILD_GUIDE.md  # This document
└── build/
    └── docker-output/     # Build outputs (created automatically)
        ├── linux-x64/
        ├── linux-arm64/
        ├── windows-x64/
        ├── macos-x64/
        └── macos-arm64/
```

### Environment Variables

**Dockerfile**:
- `DEBIAN_FRONTEND=noninteractive` - Non-interactive package installation
- `TZ=UTC` - Timezone
- `LLVM_DIR=/usr/lib/llvm-16` - LLVM installation path
- `PATH` - Include LLVM binaries

**docker-compose.yml**:
- `BUILD_TYPE=Release` - CMake build type
- `ENABLE_OPTIMIZATIONS=ON` - Enable all optimizations

### Compiler Versions

- **LLVM/Clang**: 16.x
- **GCC (aarch64)**: 11.x
- **MinGW**: 10.x
- **CMake**: 3.22+
- **Ninja**: 1.10+

---

**Last Updated**: 2025-12-04
**Nova Version**: 0.1.0
**Docker System Version**: 1.0.0
