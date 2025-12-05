#!/bin/bash
# Nova Multi-Platform Build Script
# Builds Nova for all supported platforms using Docker

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored message
print_msg() {
    echo -e "${GREEN}[Nova Build]${NC} $1"
}

print_error() {
    echo -e "${RED}[Error]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[Warning]${NC} $1"
}

# Print header
echo -e "${BLUE}"
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║                                                              ║"
echo "║        Nova Programming Language - Multi-Platform Build     ║"
echo "║                                                              ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    print_error "Docker is not installed. Please install Docker first."
    exit 1
fi

# Check if Docker Compose is installed
if ! command -v docker-compose &> /dev/null; then
    print_warning "docker-compose not found, using 'docker compose' instead"
    COMPOSE_CMD="docker compose"
else
    COMPOSE_CMD="docker-compose"
fi

# Create output directory
mkdir -p build/docker-output

# Function to build specific platform
build_platform() {
    local platform=$1
    print_msg "Building for ${platform}..."

    $COMPOSE_CMD build ${platform}

    if [ $? -eq 0 ]; then
        print_msg "✓ ${platform} build completed successfully"
        return 0
    else
        print_error "✗ ${platform} build failed"
        return 1
    fi
}

# Function to build all platforms
build_all() {
    print_msg "Building for ALL platforms..."

    $COMPOSE_CMD build build-all

    if [ $? -eq 0 ]; then
        print_msg "✓ All platforms built successfully"

        # Copy outputs
        docker run --rm \
            -v "$(pwd)/build/docker-output:/output" \
            nova-lang:multi-platform \
            sh -c "cp -r /output/* /output/"

        print_msg "Binaries copied to: build/docker-output/"

        # List built binaries
        echo -e "\n${GREEN}Built binaries:${NC}"
        find build/docker-output -name "nova*" -type f -exec ls -lh {} \;

        return 0
    else
        print_error "Build failed"
        return 1
    fi
}

# Parse command line arguments
case "${1:-all}" in
    all)
        build_all
        ;;
    linux-x64)
        build_platform "build-linux-x64"
        ;;
    linux-arm64)
        build_platform "build-linux-arm64"
        ;;
    windows-x64)
        build_platform "build-windows-x64"
        ;;
    macos-x64)
        print_warning "macOS build requires macOS SDK (see documentation)"
        build_platform "build-macos-x64"
        ;;
    macos-arm64)
        print_warning "macOS ARM64 build requires macOS SDK (see documentation)"
        build_platform "build-macos-arm64"
        ;;
    dev)
        print_msg "Starting development environment..."
        $COMPOSE_CMD run --rm dev
        ;;
    test)
        print_msg "Running tests..."
        $COMPOSE_CMD run --rm test
        ;;
    clean)
        print_msg "Cleaning build artifacts..."
        rm -rf build/docker-output
        docker rmi -f $(docker images -q nova-lang:*) 2>/dev/null || true
        print_msg "✓ Clean complete"
        ;;
    help|--help|-h)
        echo "Usage: $0 [PLATFORM]"
        echo ""
        echo "Platforms:"
        echo "  all            Build for all platforms (default)"
        echo "  linux-x64      Build for Linux x86-64"
        echo "  linux-arm64    Build for Linux ARM64"
        echo "  windows-x64    Build for Windows x86-64"
        echo "  macos-x64      Build for macOS x86-64 (requires SDK)"
        echo "  macos-arm64    Build for macOS ARM64 (requires SDK)"
        echo ""
        echo "Development:"
        echo "  dev            Start interactive development environment"
        echo "  test           Run test suite"
        echo "  clean          Remove all build artifacts"
        echo ""
        echo "Examples:"
        echo "  $0                    # Build all platforms"
        echo "  $0 linux-x64          # Build only Linux x86-64"
        echo "  $0 dev                # Start dev environment"
        ;;
    *)
        print_error "Unknown platform: $1"
        echo "Run '$0 help' for usage information"
        exit 1
        ;;
esac

exit $?
