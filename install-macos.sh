#!/bin/bash
# Nova Language - macOS Installation Script
# This script installs Nova and all required dependencies on macOS

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔═══════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                                                       ║${NC}"
echo -e "${BLUE}║           Nova Language Installer - macOS             ║${NC}"
echo -e "${BLUE}║                                                       ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════╝${NC}"
echo ""

# Detect macOS version
MACOS_VERSION=$(sw_vers -productVersion)
MACOS_NAME=$(sw_vers -productName)

echo -e "${BLUE}Detected OS:${NC} $MACOS_NAME $MACOS_VERSION"
echo ""

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install Homebrew if not present
install_homebrew() {
    if ! command_exists brew; then
        echo -e "${YELLOW}Homebrew not found. Installing Homebrew...${NC}"
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

        # Add Homebrew to PATH for Apple Silicon Macs
        if [[ $(uname -m) == 'arm64' ]]; then
            echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
            eval "$(/opt/homebrew/bin/brew shellenv)"
        fi

        echo -e "${GREEN}✓ Homebrew installed${NC}"
    else
        echo -e "${GREEN}✓ Homebrew is already installed${NC}"
    fi
}

# Function to install Xcode Command Line Tools
install_xcode_tools() {
    if ! xcode-select -p &>/dev/null; then
        echo -e "${YELLOW}Installing Xcode Command Line Tools...${NC}"
        xcode-select --install

        echo -e "${YELLOW}Please complete the Xcode Command Line Tools installation in the dialog.${NC}"
        echo -e "${YELLOW}Press Enter when done...${NC}"
        read

        echo -e "${GREEN}✓ Xcode Command Line Tools installed${NC}"
    else
        echo -e "${GREEN}✓ Xcode Command Line Tools already installed${NC}"
    fi
}

# Function to install dependencies via Homebrew
install_dependencies() {
    echo -e "${GREEN}Installing dependencies via Homebrew...${NC}"

    # Update Homebrew
    brew update

    # Install dependencies
    brew install \
        cmake \
        ninja \
        git \
        wget \
        python3 \
        openssl \
        zlib \
        libxml2

    echo -e "${GREEN}✓ Dependencies installed${NC}"
}

# Function to install LLVM 18
install_llvm() {
    echo -e "${GREEN}Checking LLVM installation...${NC}"

    if brew list llvm@18 &>/dev/null; then
        echo -e "${GREEN}✓ LLVM 18 is already installed${NC}"
        return
    fi

    echo -e "${YELLOW}Installing LLVM 18 via Homebrew...${NC}"

    # Install LLVM 18
    brew install llvm@18

    # Add LLVM to PATH
    LLVM_PATH="$(brew --prefix llvm@18)"

    # Add to shell profile
    SHELL_PROFILE=""
    if [ -n "$ZSH_VERSION" ]; then
        SHELL_PROFILE="$HOME/.zshrc"
    elif [ -n "$BASH_VERSION" ]; then
        SHELL_PROFILE="$HOME/.bash_profile"
    fi

    if [ -n "$SHELL_PROFILE" ]; then
        echo "" >> "$SHELL_PROFILE"
        echo "# LLVM 18" >> "$SHELL_PROFILE"
        echo "export PATH=\"$LLVM_PATH/bin:\$PATH\"" >> "$SHELL_PROFILE"
        echo "export LDFLAGS=\"-L$LLVM_PATH/lib \$LDFLAGS\"" >> "$SHELL_PROFILE"
        echo "export CPPFLAGS=\"-I$LLVM_PATH/include \$CPPFLAGS\"" >> "$SHELL_PROFILE"

        echo -e "${YELLOW}Added LLVM to PATH in $SHELL_PROFILE${NC}"
        echo -e "${YELLOW}Please run: source $SHELL_PROFILE${NC}"
    fi

    # Set for current session
    export PATH="$LLVM_PATH/bin:$PATH"
    export LDFLAGS="-L$LLVM_PATH/lib $LDFLAGS"
    export CPPFLAGS="-I$LLVM_PATH/include $CPPFLAGS"

    echo -e "${GREEN}✓ LLVM 18 installed${NC}"
}

# Function to build Nova
build_nova() {
    echo -e "${GREEN}Building Nova...${NC}"

    # Get LLVM path
    if command_exists brew && brew list llvm@18 &>/dev/null; then
        LLVM_PATH="$(brew --prefix llvm@18)"
        export PATH="$LLVM_PATH/bin:$PATH"
        CMAKE_PREFIX_PATH="$LLVM_PATH"
    fi

    # Configure with CMake
    cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_STANDARD=20 \
        ${CMAKE_PREFIX_PATH:+-DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH"}

    # Build
    echo -e "${BLUE}Compiling Nova (this may take a few minutes)...${NC}"
    cmake --build build -j$(sysctl -n hw.ncpu)

    echo -e "${GREEN}✓ Nova built successfully${NC}"
}

# Function to install Nova
install_nova() {
    echo -e "${GREEN}Installing Nova...${NC}"

    # Install to /usr/local/bin
    INSTALL_DIR="${INSTALL_DIR:-/usr/local}"

    sudo cmake --install build --prefix "$INSTALL_DIR"

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

        # Check if shell profile needs to be reloaded
        if [ -n "$SHELL_PROFILE" ]; then
            echo -e "${YELLOW}Note: You may need to reload your shell:${NC}"
            echo "  source $SHELL_PROFILE"
            echo ""
        fi

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
    echo "  1. Install Homebrew (if not present)"
    echo "  2. Install Xcode Command Line Tools (if not present)"
    echo "  3. Install dependencies (CMake, Ninja, etc.)"
    echo "  4. Install LLVM 18"
    echo "  5. Build Nova from source"
    echo "  6. Install Nova to /usr/local/bin"
    echo ""
    read -p "Continue? (Y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Nn]$ ]]; then
        echo "Installation cancelled."
        exit 0
    fi

    # Run installation steps
    install_homebrew
    echo ""

    install_xcode_tools
    echo ""

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
