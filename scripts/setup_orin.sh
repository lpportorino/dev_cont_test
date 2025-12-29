#!/bin/bash
# Setup script for NVIDIA Orin AGX (ARM64) native build environment
# This script installs WASI SDK, Wasmtime, and builds the OSD WASM project
#
# Usage: ./setup_orin.sh [install|build|test|all]
#   install - Install WASI SDK and Wasmtime
#   build   - Build WASM module
#   test    - Run PNG test
#   all     - Do everything (default)

set -e

# Configuration
PROJECT_DIR="/mnt/maps/jettison_wasm_osd_native"
WASI_SDK_VERSION="24"
WASI_SDK_DIR="/opt/wasi-sdk"
WASMTIME_VERSION="27.0.0"
WASMTIME_DIR="/mnt/maps/tools/wasmtime"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

check_arch() {
    ARCH=$(uname -m)
    if [ "$ARCH" != "aarch64" ]; then
        log_error "This script is for ARM64 (aarch64) only. Detected: $ARCH"
        exit 1
    fi
    log_info "Architecture: $ARCH"
}

install_wasi_sdk() {
    log_info "Installing WASI SDK $WASI_SDK_VERSION for ARM64..."

    if [ -d "$WASI_SDK_DIR" ] && [ -f "$WASI_SDK_DIR/bin/clang" ]; then
        log_info "WASI SDK already installed at $WASI_SDK_DIR"
        "$WASI_SDK_DIR/bin/clang" --version | head -1
        return 0
    fi

    cd /tmp
    WASI_SDK_TARBALL="wasi-sdk-${WASI_SDK_VERSION}.0-arm64-linux.tar.gz"
    WASI_SDK_URL="https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VERSION}/${WASI_SDK_TARBALL}"

    log_info "Downloading from $WASI_SDK_URL"
    wget -q --show-progress "$WASI_SDK_URL" -O "$WASI_SDK_TARBALL"

    log_info "Extracting WASI SDK..."
    sudo rm -rf "$WASI_SDK_DIR"
    sudo mkdir -p "$WASI_SDK_DIR"
    sudo tar -xzf "$WASI_SDK_TARBALL" -C /opt
    sudo mv /opt/wasi-sdk-${WASI_SDK_VERSION}.0-arm64-linux/* "$WASI_SDK_DIR/"
    sudo rmdir /opt/wasi-sdk-${WASI_SDK_VERSION}.0-arm64-linux

    rm -f "$WASI_SDK_TARBALL"

    log_info "WASI SDK installed to $WASI_SDK_DIR"
    "$WASI_SDK_DIR/bin/clang" --version | head -1
}

install_wasmtime() {
    log_info "Installing Wasmtime $WASMTIME_VERSION for ARM64..."

    if [ -f "$WASMTIME_DIR/wasmtime" ]; then
        log_info "Wasmtime already installed at $WASMTIME_DIR"
        "$WASMTIME_DIR/wasmtime" --version
        return 0
    fi

    mkdir -p /mnt/maps/tools

    cd /tmp
    WASMTIME_TARBALL="wasmtime-v${WASMTIME_VERSION}-aarch64-linux.tar.xz"
    WASMTIME_URL="https://github.com/bytecodealliance/wasmtime/releases/download/v${WASMTIME_VERSION}/${WASMTIME_TARBALL}"

    log_info "Downloading from $WASMTIME_URL"
    wget -q --show-progress "$WASMTIME_URL" -O "$WASMTIME_TARBALL"

    log_info "Extracting Wasmtime..."
    rm -rf "$WASMTIME_DIR"
    mkdir -p "$WASMTIME_DIR"
    tar -xJf "$WASMTIME_TARBALL" -C "$WASMTIME_DIR" --strip-components=1

    rm -f "$WASMTIME_TARBALL"

    export PATH="$WASMTIME_DIR:$PATH"

    log_info "Wasmtime installed to $WASMTIME_DIR"
    "$WASMTIME_DIR/wasmtime" --version
}

install_wasmtime_c_api() {
    log_info "Installing Wasmtime C API library..."

    WASMTIME_C_DIR="$PROJECT_DIR/deps/wasmtime-c-api"

    if [ -f "$WASMTIME_C_DIR/lib/libwasmtime.a" ]; then
        log_info "Wasmtime C API already installed at $WASMTIME_C_DIR"
        return 0
    fi

    cd /tmp
    WASMTIME_C_TARBALL="wasmtime-v${WASMTIME_VERSION}-aarch64-linux-c-api.tar.xz"
    WASMTIME_C_URL="https://github.com/bytecodealliance/wasmtime/releases/download/v${WASMTIME_VERSION}/${WASMTIME_C_TARBALL}"

    log_info "Downloading C API from $WASMTIME_C_URL"
    wget -q --show-progress "$WASMTIME_C_URL" -O "$WASMTIME_C_TARBALL"

    log_info "Extracting Wasmtime C API..."
    mkdir -p "$PROJECT_DIR/deps"
    rm -rf "$WASMTIME_C_DIR"
    mkdir -p "$WASMTIME_C_DIR"
    tar -xJf "$WASMTIME_C_TARBALL" -C "$WASMTIME_C_DIR" --strip-components=1

    rm -f "$WASMTIME_C_TARBALL"

    log_info "Wasmtime C API installed to $WASMTIME_C_DIR"
    ls -la "$WASMTIME_C_DIR/lib/"
}

check_dependencies() {
    log_info "Checking build dependencies..."

    MISSING=""

    command -v gcc &> /dev/null || MISSING="$MISSING gcc"
    command -v make &> /dev/null || MISSING="$MISSING make"
    pkg-config --exists libpng 2>/dev/null || MISSING="$MISSING libpng-dev"

    if [ -n "$MISSING" ]; then
        log_warn "Missing dependencies:$MISSING"
        log_info "Installing missing dependencies..."
        sudo apt-get update
        sudo apt-get install -y $MISSING
    else
        log_info "All dependencies present"
    fi
}

build_wasm() {
    log_info "Building WASM module..."

    cd "$PROJECT_DIR"

    if [ ! -f "src/osd_plugin.c" ]; then
        log_error "Source files not found. Please copy the project first."
        exit 1
    fi

    export WASI_SDK_PATH="$WASI_SDK_DIR"

    # Build all 4 variants
    for V in live_day live_thermal recording_day recording_thermal; do
        log_info "Building $V..."
        VARIANT="$V" BUILD_MODE=production ./tools/build.sh
    done

    log_info "Build complete!"
    ls -lh build/*.wasm
}

build_test_harness() {
    log_info "Building PNG test harness..."

    cd "$PROJECT_DIR/test"

    WASMTIME_C_DIR="$PROJECT_DIR/deps/wasmtime-c-api"

    # Compile test harness
    gcc -O2 \
        -I"$WASMTIME_C_DIR/include" \
        -I../vendor \
        -o osd_test \
        osd_test.c \
        -L"$WASMTIME_C_DIR/lib" \
        -lwasmtime \
        $(pkg-config --cflags --libs libpng) \
        -lm -lpthread -ldl

    log_info "Test harness built: $PROJECT_DIR/test/osd_test"
}

run_test() {
    log_info "Running PNG test..."

    cd "$PROJECT_DIR"
    mkdir -p snapshot

    export LD_LIBRARY_PATH="$PROJECT_DIR/deps/wasmtime-c-api/lib:$LD_LIBRARY_PATH"

    # Test each variant
    for VARIANT in live_day live_thermal recording_day recording_thermal; do
        if [ -f "build/${VARIANT}.wasm" ]; then
            log_info "Testing $VARIANT..."
            OUTPUT_PNG="snapshot/${VARIANT}.png" ./test/osd_test "build/${VARIANT}.wasm"
        fi
    done

    log_info "Test complete!"
    ls -lh snapshot/*.png 2>/dev/null || log_warn "No PNG files generated"
}

show_usage() {
    echo "Usage: $0 [install|build|test|all]"
    echo ""
    echo "Commands:"
    echo "  install  - Install WASI SDK and Wasmtime"
    echo "  build    - Build WASM modules"
    echo "  test     - Build and run PNG test harness"
    echo "  all      - Do everything (default)"
    echo ""
    echo "Environment:"
    echo "  WASI_SDK_DIR: $WASI_SDK_DIR"
    echo "  WASMTIME_DIR: $WASMTIME_DIR"
    echo "  PROJECT_DIR:  $PROJECT_DIR"
}

# Main
cd "$PROJECT_DIR" 2>/dev/null || true

case "${1:-all}" in
    install)
        check_arch
        check_dependencies
        install_wasi_sdk
        install_wasmtime
        install_wasmtime_c_api
        ;;
    build)
        check_arch
        build_wasm
        ;;
    test)
        check_arch
        build_test_harness
        run_test
        ;;
    all)
        check_arch
        check_dependencies
        install_wasi_sdk
        install_wasmtime
        install_wasmtime_c_api
        build_wasm
        build_test_harness
        run_test
        ;;
    -h|--help|help)
        show_usage
        ;;
    *)
        log_error "Unknown command: $1"
        show_usage
        exit 1
        ;;
esac

log_info "Done!"
