#!/bin/bash
# Post-create script for Jettison WASM OSD Dev Container
# Runs once after container is created
# Initializes git submodules and installs npm dependencies

set -e  # Exit on error

echo "=================================="
echo "🔧 Running post-create setup..."
echo "=================================="

# Change to workspace directory
cd /workspace

# ============================================================================
# 0. Fix Named Volume Permissions
# ============================================================================
# Named volumes are created with root ownership. Fix permissions so vscode user
# can write to them. This must run BEFORE any other steps that write to volumes.
echo ""
echo "🔐 Fixing named volume permissions..."

# Only node_modules is a named volume now (others are part of workspace bind mount)
VOLUME_DIRS="tools/snapshot/node_modules"

for dir in $VOLUME_DIRS; do
    if [ -d "$dir" ]; then
        # Check if we can write to the directory
        if ! touch "$dir/.write_test" 2>/dev/null; then
            echo "   Fixing permissions for $dir..."
            sudo chown -R "$(id -u):$(id -g)" "$dir"
        fi
        rm -f "$dir/.write_test" 2>/dev/null
    fi
done

echo "✅ Volume permissions fixed"

# ============================================================================
# 0.1. Patch WASI SDK for CMake 4.x Compatibility
# ============================================================================
# CLion 2025.3+ bundles CMake 4.1 which removed support for cmake_minimum_required < 3.5
# WASI SDK uses VERSION 3.4.0 which causes: "Compatibility with CMake < 3.5 has been removed"
echo ""
echo "🔧 Patching WASI SDK for CMake 4.x compatibility..."

WASI_CMAKE="/opt/wasi-sdk/share/cmake/wasi-sdk.cmake"
if [ -f "$WASI_CMAKE" ]; then
    # Check for any version < 3.10 and update to 3.10 (CMake 4.x requirement)
    if grep -qE "cmake_minimum_required\(VERSION 3\.[0-9](\.[0-9])?\)" "$WASI_CMAKE" 2>/dev/null; then
        sudo sed -i 's/cmake_minimum_required(VERSION 3\.[0-9]\(\.[0-9]\)\?)/cmake_minimum_required(VERSION 3.10)/' "$WASI_CMAKE"
        echo "✅ Patched wasi-sdk.cmake for CMake 4.x compatibility (VERSION 3.10)"
    else
        echo "✅ wasi-sdk.cmake already compatible"
    fi
else
    echo "⚠️  WASI SDK cmake file not found at $WASI_CMAKE"
fi

# ============================================================================
# 0.5. Configure SSH Agent Environment
# ============================================================================
# SSH agent is forwarded via remoteEnv in devcontainer.json
# CLion users: Enable in Settings > Build > Dev Containers > Forward SSH agent
echo ""
echo "🔑 Checking SSH agent..."

# Check if SSH_AUTH_SOCK is set and points to a valid socket
if [ -n "$SSH_AUTH_SOCK" ] && [ -S "$SSH_AUTH_SOCK" ]; then
    # Test SSH agent
    if ssh-add -l >/dev/null 2>&1; then
        echo "✅ SSH agent connected ($(ssh-add -l | wc -l) keys loaded)"
    else
        echo "⚠️  SSH agent socket exists but no keys loaded"
        echo "   Run 'ssh-add' on host to add your key"
    fi
else
    echo "ℹ️  SSH agent not forwarded (optional for builds)"
    echo "   CLI: export SSH_AUTH_SOCK before devcontainer commands"
    echo "   CLion: Settings > Build > Dev Containers > Forward SSH agent"
fi

# ============================================================================
# 0.6. Add GitHub to Known Hosts
# ============================================================================
# Prevent "Host key verification failed" when cloning submodules
echo ""
echo "🌐 Configuring SSH known hosts..."

mkdir -p ~/.ssh
chmod 700 ~/.ssh

if ! grep -q 'github.com' ~/.ssh/known_hosts 2>/dev/null; then
    ssh-keyscan -t ed25519,rsa github.com >> ~/.ssh/known_hosts 2>/dev/null
    echo "✅ Added GitHub to known_hosts"
else
    echo "✅ GitHub already in known_hosts"
fi

# ============================================================================
# 0.7. Configure Git Identity (if not set)
# ============================================================================
# Set git identity inside container if not already configured
# This avoids needing to bind mount ~/.gitconfig from host
echo ""
echo "🔧 Configuring git identity..."

# Check if git user is already configured
if git config --global user.email > /dev/null 2>&1; then
    echo "✅ Git identity already configured: $(git config --global user.name) <$(git config --global user.email)>"
else
    # Try to get from the repo's git log
    if [ -d ".git" ]; then
        REPO_EMAIL=$(git log -1 --format='%ae' 2>/dev/null || true)
        REPO_NAME=$(git log -1 --format='%an' 2>/dev/null || true)
        if [ -n "$REPO_EMAIL" ] && [ -n "$REPO_NAME" ]; then
            git config --global user.email "$REPO_EMAIL"
            git config --global user.name "$REPO_NAME"
            echo "✅ Git identity set from repo history: $REPO_NAME <$REPO_EMAIL>"
        else
            echo "⚠️  No git identity configured"
            echo "   Run: git config --global user.email 'you@example.com'"
            echo "        git config --global user.name 'Your Name'"
        fi
    else
        echo "⚠️  No git identity configured and not a git repo"
    fi
fi

# ============================================================================
# 1. Initialize Git Submodules
# ============================================================================
echo ""
echo "📦 Initializing git submodules..."

if [ -d ".git" ]; then
    # Check if submodules are already initialized
    if ! git submodule status | grep -q '^-'; then
        echo "✅ Git submodules already initialized"
    else
        echo "⬇️  Fetching proto submodules (C and TypeScript bindings)..."
        # Use --remote to fetch latest from remote if exact commit not found
        if ! git submodule update --init --recursive proto/c proto/ts 2>&1; then
            echo "⚠️  Some submodules failed to initialize exactly as recorded"
            echo "   Attempting to fetch latest from remote branches..."
            git submodule update --init --remote --recursive proto/c proto/ts 2>&1 || true
            echo "⚠️  Submodule initialization had issues - build may still work"
            echo "   Run 'git submodule status' to check state"
        else
            echo "✅ Git submodules initialized successfully"
        fi
    fi
else
    echo "⚠️  Not a git repository - skipping submodule initialization"
fi

# ============================================================================
# 2. Install npm dependencies for snapshot tool
# ============================================================================
echo ""
echo "📦 Installing npm dependencies for snapshot tool..."

if [ -f "tools/snapshot/package.json" ]; then
    cd tools/snapshot

    # Check if node_modules already exists and has packages
    if [ -d "node_modules" ] && [ -n "$(ls -A node_modules 2>/dev/null)" ]; then
        echo "✅ npm dependencies already installed"
    else
        echo "⬇️  Running npm install..."
        npm install --quiet
        echo "✅ npm dependencies installed successfully"
    fi

    cd /workspace
else
    echo "⚠️  tools/snapshot/package.json not found - skipping npm install"
fi

# ============================================================================
# 3. Create necessary output directories (if they don't exist in volumes)
# ============================================================================
echo ""
echo "📁 Ensuring output directories exist..."

mkdir -p build
mkdir -p dist
mkdir -p test/output
mkdir -p snapshot

echo "✅ Output directories ready"

# ============================================================================
# 4. Display environment information
# ============================================================================
echo ""
echo "=================================="
echo "🎉 Post-create setup complete!"
echo "=================================="
echo ""
echo "📊 Environment Information:"
echo "   WASI SDK: $(${WASI_SDK_PATH}/bin/clang --version | head -n1)"
echo "   Wasmtime: $(wasmtime --version)"
echo "   Node.js: $(node --version)"
echo "   npm: $(npm --version)"
echo ""
echo "🚀 Next steps:"
echo "   - Run 'make help' to see all available commands"
echo "   - Run 'make docker-build' to build all 4 WASM variants"
echo "   - Run 'make docker-test' to generate PNG test outputs"
echo "   - Run 'make video' to generate all 4 test videos"
echo ""
echo "📖 Documentation:"
echo "   - See CLAUDE.md for project architecture and development guide"
echo "   - See README.md for quick start and usage"
echo ""
