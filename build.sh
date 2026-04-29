#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# Couch LA-2A – build script
# Usage:  ./build.sh          (configures + builds)
#         ./build.sh deps     (installs system dependencies, requires sudo)
#         ./build.sh clean    (removes build directory)
# ─────────────────────────────────────────────────────────────────────────────
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

install_deps() {
    echo "→ Installing system dependencies..."
    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        cmake \
        git \
        libasound2-dev \
        libx11-dev \
        libxext-dev \
        libxinerama-dev \
        libxrandr-dev \
        libxcursor-dev \
        libxrender-dev \
        libfreetype6-dev \
        libfontconfig1-dev \
        libgl1-mesa-dev \
        libglu1-mesa-dev \
        pkg-config
    echo "✓ Dependencies installed."
}

clean_build() {
    echo "→ Removing $BUILD_DIR ..."
    rm -rf "$BUILD_DIR"
    echo "✓ Clean complete."
}

build() {
    echo "─────────────────────────────────────────────────────"
    echo "  Couch LA-2A – Optical Leveling Amplifier"
    echo "─────────────────────────────────────────────────────"

    if ! command -v cmake &>/dev/null; then
        echo "ERROR: cmake not found. Run: ./build.sh deps"
        exit 1
    fi

    echo "→ Configuring with CMake..."
    cmake -S "$SCRIPT_DIR" \
          -B "$BUILD_DIR" \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

    echo "→ Building (will reuse JUCE cache if already downloaded)..."
    cmake --build "$BUILD_DIR" --config Release --parallel "$(nproc)"

    echo ""
    echo "─────────────────────────────────────────────────────"
    echo "  ✓ Build complete!"
    echo ""
    echo "  VST3 plugin:"
    find "$BUILD_DIR" -name "*.vst3" -type d 2>/dev/null | head -3 | \
        while read -r p; do echo "    $p"; done
    echo ""
    echo "  Standalone app:"
    find "$BUILD_DIR" -name "Couch LA-2A" 2>/dev/null | head -3 | \
        while read -r p; do echo "    $p"; done
    echo ""
    echo "  To install the VST3 system-wide:"
    echo "    cp -r <path>.vst3 ~/.vst3/"
    echo "─────────────────────────────────────────────────────"
}

case "${1:-build}" in
    deps)  install_deps ;;
    clean) clean_build  ;;
    *)     build        ;;
esac
