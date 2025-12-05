#!/bin/bash
# Nova Installer - One-liner installation script
# Usage: curl -fsSL https://nova-lang.org/install.sh | bash

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

NOVA_VERSION="${NOVA_VERSION:-latest}"
GITHUB_REPO="d-osc/Nova"
INSTALL_DIR="${NOVA_INSTALL_DIR:-$HOME/.nova}"
BIN_DIR="$INSTALL_DIR/bin"

echo -e "${CYAN}"
echo "┌─────────────────────────────────────┐"
echo "│         Nova Installer              │"
echo "│   TypeScript to Native Compiler     │"
echo "└─────────────────────────────────────┘"
echo -e "${NC}"

# Detect OS and architecture
detect_platform() {
    OS="$(uname -s)"
    ARCH="$(uname -m)"

    case "$OS" in
        Linux*)
            PLATFORM="linux"
            ;;
        Darwin*)
            PLATFORM="macos"
            ;;
        *)
            echo -e "${RED}✗ Unsupported OS: $OS${NC}"
            exit 1
            ;;
    esac

    case "$ARCH" in
        x86_64|amd64)
            ARCH_NAME="x64"
            ;;
        aarch64|arm64)
            ARCH_NAME="arm64"
            ;;
        *)
            echo -e "${RED}✗ Unsupported architecture: $ARCH${NC}"
            exit 1
            ;;
    esac

    echo -e "${BLUE}→ Detected platform: ${GREEN}$PLATFORM-$ARCH_NAME${NC}"
}

# Download Nova binary
download_nova() {
    echo -e "${BLUE}→ Downloading Nova...${NC}"

    # Determine download URL based on platform
    if [ "$NOVA_VERSION" = "latest" ]; then
        DOWNLOAD_URL="https://github.com/$GITHUB_REPO/releases/latest/download/nova-$PLATFORM-$ARCH_NAME.zip"
    else
        DOWNLOAD_URL="https://github.com/$GITHUB_REPO/releases/download/$NOVA_VERSION/nova-$PLATFORM-$ARCH_NAME.zip"
    fi

    # Download and extract zip file
    TMP_ZIP=$(mktemp -u).zip
    TMP_DIR=$(mktemp -d)

    if command -v curl >/dev/null 2>&1; then
        curl -fsSL "$DOWNLOAD_URL" -o "$TMP_ZIP"
    elif command -v wget >/dev/null 2>&1; then
        wget -q "$DOWNLOAD_URL" -O "$TMP_ZIP"
    else
        echo -e "${RED}✗ Error: curl or wget is required${NC}"
        exit 1
    fi

    echo -e "${BLUE}→ Extracting Nova...${NC}"

    # Extract zip file
    if command -v unzip >/dev/null 2>&1; then
        unzip -q "$TMP_ZIP" -d "$TMP_DIR"
    else
        echo -e "${RED}✗ Error: unzip is required${NC}"
        exit 1
    fi

    # Find the extracted binary and move it
    mkdir -p "$BIN_DIR"
    EXTRACTED_BINARY=$(find "$TMP_DIR" -type f -name "nova-*" | head -n 1)

    if [ -n "$EXTRACTED_BINARY" ]; then
        mv "$EXTRACTED_BINARY" "$BIN_DIR/nova"
    else
        echo -e "${RED}✗ Error: Could not find extracted binary${NC}"
        exit 1
    fi

    # Cleanup
    rm -f "$TMP_ZIP"
    rm -rf "$TMP_DIR"

    chmod +x "$BIN_DIR/nova"
    echo -e "${GREEN}✓ Nova downloaded and extracted successfully${NC}"
}

# Setup PATH
setup_path() {
    echo -e "${BLUE}→ Setting up PATH...${NC}"

    # Detect shell
    SHELL_NAME=$(basename "$SHELL")
    case "$SHELL_NAME" in
        bash)
            PROFILE_FILE="$HOME/.bashrc"
            ;;
        zsh)
            PROFILE_FILE="$HOME/.zshrc"
            ;;
        fish)
            PROFILE_FILE="$HOME/.config/fish/config.fish"
            ;;
        *)
            PROFILE_FILE="$HOME/.profile"
            ;;
    esac

    # Add to PATH if not already present
    EXPORT_LINE="export PATH=\"$BIN_DIR:\$PATH\""

    if [ "$SHELL_NAME" = "fish" ]; then
        EXPORT_LINE="set -gx PATH $BIN_DIR \$PATH"
    fi

    if [ -f "$PROFILE_FILE" ]; then
        if ! grep -q "$BIN_DIR" "$PROFILE_FILE" 2>/dev/null; then
            echo "" >> "$PROFILE_FILE"
            echo "# Nova" >> "$PROFILE_FILE"
            echo "$EXPORT_LINE" >> "$PROFILE_FILE"
            echo -e "${GREEN}✓ Added Nova to PATH in $PROFILE_FILE${NC}"
        else
            echo -e "${YELLOW}! Nova is already in PATH${NC}"
        fi
    else
        echo "" >> "$PROFILE_FILE"
        echo "# Nova" >> "$PROFILE_FILE"
        echo "$EXPORT_LINE" >> "$PROFILE_FILE"
        echo -e "${GREEN}✓ Created $PROFILE_FILE and added Nova to PATH${NC}"
    fi

    # Export for current session
    export PATH="$BIN_DIR:$PATH"
}

# Verify installation
verify_installation() {
    echo -e "${BLUE}→ Verifying installation...${NC}"

    if [ -x "$BIN_DIR/nova" ]; then
        VERSION=$("$BIN_DIR/nova" --version 2>/dev/null || echo "unknown")
        echo -e "${GREEN}✓ Nova installed successfully!${NC}"
        echo -e "${CYAN}  Version: $VERSION${NC}"
        echo -e "${CYAN}  Location: $BIN_DIR/nova${NC}"
    else
        echo -e "${RED}✗ Installation verification failed${NC}"
        exit 1
    fi
}

# Print next steps
print_next_steps() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║             Installation Complete! 🎉              ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${YELLOW}To start using Nova, run:${NC}"
    echo ""
    echo -e "  ${GREEN}source $(basename "$PROFILE_FILE")${NC}  # Load the new PATH"
    echo -e "  ${GREEN}nova --version${NC}             # Verify installation"
    echo -e "  ${GREEN}nova --help${NC}                # See available commands"
    echo ""
    echo -e "${YELLOW}Quick start:${NC}"
    echo -e "  ${GREEN}nova run app.ts${NC}            # Run TypeScript file"
    echo -e "  ${GREEN}nova build app.ts -o app${NC}   # Compile to binary"
    echo ""
    echo -e "${BLUE}Documentation: ${NC}https://github.com/$GITHUB_REPO"
    echo ""
}

# Main installation flow
main() {
    detect_platform
    download_nova
    setup_path
    verify_installation
    print_next_steps
}

main
