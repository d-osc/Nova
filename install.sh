#!/bin/bash
# Nova Language - Universal Installation Script
# Automatically detects OS and runs the appropriate installer

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}╔═══════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                                                       ║${NC}"
echo -e "${BLUE}║           Nova Language Universal Installer           ║${NC}"
echo -e "${BLUE}║                                                       ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════╝${NC}"
echo ""

# Detect operating system
detect_os() {
    case "$(uname -s)" in
        Linux*)
            echo "linux"
            ;;
        Darwin*)
            echo "macos"
            ;;
        CYGWIN*|MINGW*|MSYS*|MINGW32*|MINGW64*)
            echo "windows"
            ;;
        *)
            echo "unknown"
            ;;
    esac
}

OS=$(detect_os)

echo -e "${BLUE}Detected Operating System:${NC} $OS"
echo ""

# Run appropriate installer
case "$OS" in
    linux)
        echo -e "${GREEN}Running Linux installer...${NC}"
        echo ""
        if [ -f "./install-linux.sh" ]; then
            chmod +x ./install-linux.sh
            exec ./install-linux.sh "$@"
        else
            echo -e "${RED}Error: install-linux.sh not found${NC}"
            echo "Please make sure you're running this from the Nova source directory."
            exit 1
        fi
        ;;

    macos)
        echo -e "${GREEN}Running macOS installer...${NC}"
        echo ""
        if [ -f "./install-macos.sh" ]; then
            chmod +x ./install-macos.sh
            exec ./install-macos.sh "$@"
        else
            echo -e "${RED}Error: install-macos.sh not found${NC}"
            echo "Please make sure you're running this from the Nova source directory."
            exit 1
        fi
        ;;

    windows)
        echo -e "${YELLOW}Detected Windows (Git Bash/MSYS)${NC}"
        echo ""
        echo -e "${YELLOW}Please run the Windows installer directly:${NC}"
        echo "  powershell -ExecutionPolicy Bypass -File install-windows.ps1"
        echo ""
        echo "Or in PowerShell as Administrator:"
        echo "  .\\install-windows.ps1"
        exit 0
        ;;

    unknown)
        echo -e "${RED}Error: Unknown operating system${NC}"
        echo ""
        echo "Supported operating systems:"
        echo "  - Linux (Ubuntu, Debian, Fedora, CentOS, Arch)"
        echo "  - macOS (10.15+)"
        echo "  - Windows 10/11"
        echo ""
        echo "Manual installation:"
        echo "  1. Install dependencies: CMake, Ninja, LLVM 18"
        echo "  2. Run: cmake -B build -G Ninja"
        echo "  3. Run: cmake --build build"
        echo "  4. Run: cmake --install build"
        exit 1
        ;;
esac
