#!/bin/bash
# Post-start script for Jettison WASM OSD Dev Container
# Runs every time the container starts
# Optional: Add any startup tasks here

set -e  # Exit on error

# Display a welcome message with quick tips
echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Jettison WASM OSD C Development Container                    ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "📍 Working directory: /workspace"
echo "🛠️  Toolchains available:"
echo "   - WASI SDK (C → WASM compiler)"
echo "   - Wasmtime (WASM runtime)"
echo "   - GStreamer (video generation)"
echo "   - Node.js + TypeScript (snapshot tool)"
echo ""
echo "⚡ Quick commands:"
echo "   make help                  - Show all available commands"
echo "   make docker-build          - Build all 4 WASM variants"
echo "   make docker-package        - Build + package all variants"
echo "   make video                 - Generate test videos"
echo "   make format                - Format C source files"
echo "   make lint                  - Run clang-tidy linter"
echo ""

# Optional: Check if build artifacts exist and display status
if [ -f "build/live_day.wasm" ] || [ -f "build/recording_day.wasm" ]; then
    echo "✅ WASM artifacts found in build/"
else
    echo "ℹ️  No WASM artifacts yet - run 'make docker-build' to compile"
fi

echo ""
