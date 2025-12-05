# Nova Programming Language - Multi-Platform Build Container
# Supports: Linux (x86-64, ARM64), Windows (x86-64), macOS (x86-64, ARM64)

FROM ubuntu:22.04 AS base

# Prevent interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Install base dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    ninja-build \
    python3 \
    python3-pip \
    pkg-config \
    libssl-dev \
    ca-certificates \
    lsb-release \
    software-properties-common \
    gnupg \
    libffi-dev \
    libtinfo-dev \
    libncurses5-dev \
    libz-dev \
    libzstd-dev \
    libcurl4-openssl-dev \
    libxml2-dev \
    && rm -rf /var/lib/apt/lists/*

# Install LLVM 16 (matches Nova's LLVM version)
RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 16 && \
    rm llvm.sh

# Set LLVM environment variables
ENV LLVM_DIR=/usr/lib/llvm-16
ENV PATH="${LLVM_DIR}/bin:${PATH}"
ENV LD_LIBRARY_PATH="${LLVM_DIR}/lib:${LD_LIBRARY_PATH}"

# ==============================================================================
# Stage: Cross-Compilation Tools
# ==============================================================================

FROM base AS cross-compile-tools

# Install cross-compilation tools for multiple architectures
RUN apt-get update && apt-get install -y \
    # ARM64 cross-compilation
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    binutils-aarch64-linux-gnu \
    # Windows cross-compilation (MinGW)
    mingw-w64 \
    wine64 \
    # macOS cross-compilation tools
    clang \
    lld \
    # Additional tools
    qemu-user-static \
    && rm -rf /var/lib/apt/lists/*

# Install osxcross for macOS cross-compilation
WORKDIR /opt
RUN git clone https://github.com/tpoechtrager/osxcross.git && \
    cd osxcross && \
    # Note: You need to provide MacOSX SDK separately
    # Download from https://github.com/joseluisq/macosx-sdks
    echo "macOS SDK setup requires manual SDK file (see documentation)"

# ==============================================================================
# Stage: Nova Build Environment
# ==============================================================================

FROM cross-compile-tools AS nova-build

# Create build directory
WORKDIR /nova

# Copy Nova source code
COPY . /nova/

# Create build output directories
RUN mkdir -p /nova/build/linux-x64 \
    /nova/build/linux-arm64 \
    /nova/build/windows-x64 \
    /nova/build/macos-x64 \
    /nova/build/macos-arm64

# ==============================================================================
# Stage: Linux x86-64 Build
# ==============================================================================

FROM nova-build AS build-linux-x64

WORKDIR /nova/build/linux-x64

RUN cmake ../.. \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang-16 \
    -DCMAKE_CXX_COMPILER=clang++-16 \
    -DCMAKE_CXX_FLAGS="-O3 -march=x86-64-v3 -mavx2 -DNDEBUG" \
    -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
    && ninja nova

# Strip binary for smaller size
RUN strip nova || true

# ==============================================================================
# Stage: Linux ARM64 Build
# ==============================================================================

FROM nova-build AS build-linux-arm64

WORKDIR /nova/build/linux-arm64

RUN cmake ../.. \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
    -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
    -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
    -DCMAKE_CXX_FLAGS="-O3 -march=armv8-a+simd -DNDEBUG" \
    && ninja nova || echo "ARM64 build may require LLVM ARM64 support"

# ==============================================================================
# Stage: Windows x86-64 Build (MinGW)
# ==============================================================================

FROM nova-build AS build-windows-x64

WORKDIR /nova/build/windows-x64

RUN cmake ../.. \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_CXX_FLAGS="-O3 -march=x86-64 -DNDEBUG" \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" \
    && ninja nova || echo "Windows build may require additional configuration"

# ==============================================================================
# Stage: macOS x86-64 Build (requires SDK)
# ==============================================================================

FROM nova-build AS build-macos-x64

WORKDIR /nova/build/macos-x64

# Note: This requires macOS SDK to be present
RUN if [ -d "/opt/osxcross/target" ]; then \
    export PATH="/opt/osxcross/target/bin:$PATH" && \
    cmake ../.. \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SYSTEM_NAME=Darwin \
        -DCMAKE_OSX_ARCHITECTURES=x86_64 \
        -DCMAKE_C_COMPILER=o64-clang \
        -DCMAKE_CXX_COMPILER=o64-clang++ \
        -DCMAKE_CXX_FLAGS="-O3 -march=x86-64 -DNDEBUG" \
        && ninja nova; \
    else \
        echo "macOS SDK not found, skipping macOS build"; \
    fi

# ==============================================================================
# Stage: macOS ARM64 Build (Apple Silicon)
# ==============================================================================

FROM nova-build AS build-macos-arm64

WORKDIR /nova/build/macos-arm64

# Note: This requires macOS SDK to be present
RUN if [ -d "/opt/osxcross/target" ]; then \
    export PATH="/opt/osxcross/target/bin:$PATH" && \
    cmake ../.. \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SYSTEM_NAME=Darwin \
        -DCMAKE_OSX_ARCHITECTURES=arm64 \
        -DCMAKE_C_COMPILER=oa64-clang \
        -DCMAKE_CXX_COMPILER=oa64-clang++ \
        -DCMAKE_CXX_FLAGS="-O3 -march=armv8-a -DNDEBUG" \
        && ninja nova; \
    else \
        echo "macOS SDK not found, skipping macOS ARM64 build"; \
    fi

# ==============================================================================
# Stage: Collect All Binaries
# ==============================================================================

FROM ubuntu:22.04 AS final

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libstdc++6 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create output directory structure
RUN mkdir -p /output/linux-x64 \
    /output/linux-arm64 \
    /output/windows-x64 \
    /output/macos-x64 \
    /output/macos-arm64

# Copy built binaries from each platform stage
COPY --from=build-linux-x64 /nova/build/linux-x64/ /tmp/linux-x64/
COPY --from=build-linux-arm64 /nova/build/linux-arm64/ /tmp/linux-arm64/
COPY --from=build-windows-x64 /nova/build/windows-x64/ /tmp/windows-x64/
COPY --from=build-macos-x64 /nova/build/macos-x64/ /tmp/macos-x64/
COPY --from=build-macos-arm64 /nova/build/macos-arm64/ /tmp/macos-arm64/

# Copy binaries to output (with error handling)
RUN cp /tmp/linux-x64/nova /output/linux-x64/ 2>/dev/null || echo "Linux x64: No binary" && \
    cp /tmp/linux-arm64/nova /output/linux-arm64/ 2>/dev/null || echo "Linux ARM64: No binary" && \
    cp /tmp/windows-x64/nova.exe /output/windows-x64/ 2>/dev/null || echo "Windows: No binary" && \
    cp /tmp/macos-x64/nova /output/macos-x64/ 2>/dev/null || echo "macOS x64: No binary (SDK required)" && \
    cp /tmp/macos-arm64/nova /output/macos-arm64/ 2>/dev/null || echo "macOS ARM64: No binary (SDK required)"

# Create build info file
RUN echo "Nova Programming Language - Multi-Platform Build" > /output/BUILD_INFO.txt && \
    echo "Build Date: $(date -u)" >> /output/BUILD_INFO.txt && \
    echo "Supported Platforms:" >> /output/BUILD_INFO.txt && \
    echo "  - Linux x86-64" >> /output/BUILD_INFO.txt && \
    echo "  - Linux ARM64" >> /output/BUILD_INFO.txt && \
    echo "  - Windows x86-64" >> /output/BUILD_INFO.txt && \
    echo "  - macOS x86-64" >> /output/BUILD_INFO.txt && \
    echo "  - macOS ARM64 (Apple Silicon)" >> /output/BUILD_INFO.txt

WORKDIR /output

# Default command: list built binaries
CMD ["sh", "-c", "find /output -name 'nova*' -type f -exec ls -lh {} \\; && cat /output/BUILD_INFO.txt"]
