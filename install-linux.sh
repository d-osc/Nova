#!/bin/bash
# Nova Language - Linux Installation Script
# This script installs Nova and all required dependencies on Linux

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔═══════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                                                       ║${NC}"
echo -e "${BLUE}║           Nova Language Installer - Linux            ║${NC}"
echo -e "${BLUE}║                                                       ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════╝${NC}"
echo ""

# Detect Linux distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$NAME
    VER=$VERSION_ID
else
    echo -e "${RED}Cannot detect Linux distribution${NC}"
    exit 1
fi

echo -e "${BLUE}Detected OS:${NC} $OS $VER"
echo ""

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    echo -e "${YELLOW}Warning: Running as root. This will install system-wide.${NC}"
    SUDO=""
else
    SUDO="sudo"
fi

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install dependencies based on distro
install_dependencies() {
    echo -e "${GREEN}Installing dependencies...${NC}"

    case "$OS" in
        *"Ubuntu"*|*"Debian"*)
            $SUDO apt-get update
            $SUDO apt-get install -y \
                build-essential \
                cmake \
                ninja-build \
                git \
                wget \
                curl \
                python3 \
                python3-pip \
                libssl-dev \
                zlib1g-dev \
                libxml2-dev \
                libedit-dev
            ;;
        *"Fedora"*|*"Red Hat"*|*"CentOS"*)
            $SUDO dnf install -y \
                gcc \
                gcc-c++ \
                cmake \
                ninja-build \
                git \
                wget \
                curl \
                python3 \
                python3-pip \
                openssl-devel \
                zlib-devel \
                libxml2-devel \
                libedit-devel
            ;;
        *"Arch"*|*"Manjaro"*)
            $SUDO pacman -Sy --noconfirm \
                base-devel \
                cmake \
                ninja \
                git \
                wget \
                curl \
                python \
                python-pip \
                openssl \
                zlib \
                libxml2 \
                libedit
            ;;
        *)
            echo -e "${YELLOW}Unknown distribution. Please install dependencies manually:${NC}"
            echo "  - GCC/Clang (C++20 support)"
            echo "  - CMake (>= 3.20)"
            echo "  - Ninja"
            echo "  - Git"
            echo "  - Python 3"
            echo "  - OpenSSL, zlib, libxml2, libedit"
            read -p "Continue anyway? (y/N): " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                exit 1
            fi
            ;;
    esac
}

# Function to install LLVM 18
install_llvm() {
    echo -e "${GREEN}Checking LLVM installation...${NC}"

    if command_exists llvm-config && llvm-config --version | grep -q "^18"; then
        echo -e "${GREEN}✓ LLVM 18 is already installed${NC}"
        return
    fi

    echo -e "${YELLOW}Installing LLVM 18...${NC}"

    # Try to install from package manager first
    case "$OS" in
        *"Ubuntu"*|*"Debian"*)
            # Add LLVM repository
            wget https://apt.llvm.org/llvm.sh
            chmod +x llvm.sh
            $SUDO ./llvm.sh 18
            rm llvm.sh
            ;;
        *"Fedora"*|*"Red Hat"*|*"CentOS"*)
            $SUDO dnf install -y llvm18 llvm18-devel clang18
            ;;
        *"Arch"*|*"Manjaro"*)
            $SUDO pacman -Sy --noconfirm llvm clang
            ;;
        *)
            echo -e "${YELLOW}Please install LLVM 18 manually from: https://llvm.org/${NC}"
            exit 1
            ;;
    esac

    echo -e "${GREEN}✓ LLVM 18 installed${NC}"
}

# Function to build Nova
build_nova() {
    echo -e "${GREEN}Building Nova...${NC}"

    # Configure with CMake
    cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_STANDARD=20

    # Build
    echo -e "${BLUE}Compiling Nova (this may take a few minutes)...${NC}"
    cmake --build build -j$(nproc)

    echo -e "${GREEN}✓ Nova built successfully${NC}"
}

# Function to install Nova
install_nova() {
    echo -e "${GREEN}Installing Nova...${NC}"

    # Install to /usr/local/bin (or user-specified location)
    INSTALL_DIR="${INSTALL_DIR:-/usr/local}"

    $SUDO cmake --install build --prefix "$INSTALL_DIR"

    # Create symlink if not in PATH
    if [ ! -f "/usr/local/bin/nova" ] && [ -f "$INSTALL_DIR/bin/nova" ]; then
        $SUDO ln -sf "$INSTALL_DIR/bin/nova" /usr/local/bin/nova
    fi

    echo -e "${GREEN}✓ Nova installed to $INSTALL_DIR${NC}"
}

# Function to verify installation
verify_installation() {
    echo -e "${GREEN}Verifying installation...${NC}"

    if command_exists nova; then
        NOVA_VERSION=$(nova --version 2>&1 || echo "unknown")
        echo -e "${GREEN}✓ Nova is installed: $NOVA_VERSION${NC}"
        echo ""
        echo -e "${BLUE}Installation successful!${NC}"
        echo ""
        echo -e "${YELLOW}Quick start:${NC}"
        echo "  nova --version          # Check version"
        echo "  nova --help             # Show help"
        echo "  nova run hello.ts       # Run a TypeScript file"
        echo "  nova build app.ts       # Build to executable"
        echo ""
        return 0
    else
        echo -e "${RED}✗ Nova installation failed${NC}"
        echo "Please check the error messages above."
        return 1
    fi
}

# Main installation flow
main() {
    echo -e "${BLUE}Starting installation...${NC}"
    echo ""

    # Check if we're in the Nova source directory
    if [ ! -f "CMakeLists.txt" ]; then
        echo -e "${RED}Error: This script must be run from the Nova source directory${NC}"
        exit 1
    fi

    # Ask for confirmation
    echo -e "${YELLOW}This script will:${NC}"
    echo "  1. Install system dependencies"
    echo "  2. Install LLVM 18"
    echo "  3. Build Nova from source"
    echo "  4. Install Nova to /usr/local/bin"
    echo ""
    read -p "Continue? (Y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Nn]$ ]]; then
        echo "Installation cancelled."
        exit 0
    fi

    # Run installation steps
    install_dependencies
    echo ""

    install_llvm
    echo ""

    build_nova
    echo ""

    install_nova
    echo ""

    verify_installation
}

# Run main installation
main
