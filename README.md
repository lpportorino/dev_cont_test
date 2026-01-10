# Jettison WASM OSD

C-to-WebAssembly OSD (On-Screen Display) plugin for Jettison. Compiles to WASM using WASI SDK and renders telemetry overlays including crosshair, navball, timestamp, and speed indicators.

**Key Features**: Quaternion-based navball rotation, celestial body indicators, and cryptographic package signing.

## Table of Contents

- [Quick Start](#quick-start)
- [Development Workflows](#development-workflows)
  - [Script-Based (Recommended)](#script-based-recommended)
  - [CLion / IDE with Dev Container](#clion--ide-with-dev-container)
- [Visual Debugging](#visual-debugging)
- [Build Artifacts](#build-artifacts)
- [Project Structure](#project-structure)
- [Variants](#variants)
- [Integration with prototype_basic](#integration-with-prototype_basic)
- [Technical Details](#technical-details)

---

## Quick Start

**Prerequisites**: Docker (running) + npm (for @devcontainers/cli, auto-installed if missing)

```bash
# Build all 4 production packages (~960KB each)
./tools/devcontainer-build.sh package

# Generate visual snapshots to verify rendering
./tools/devcontainer-build.sh test-png

# Open files in dist/ to inspect output
```

---

## Development Workflows

Choose between **script-based** development (recommended for most users) or **IDE-based** development with CLion's Dev Container integration.

### Script-Based (Recommended)

The `./tools/devcontainer-build.sh` script handles everything from the host machine. Docker runs the build container automatically.

#### Common Commands

| Command | Description | Output |
|---------|-------------|--------|
| `./tools/devcontainer-build.sh package` | Build + sign 4 production packages | `dist/*.tar` |
| `./tools/devcontainer-build.sh package-dev` | Build + sign 4 debug packages | `dist/*-dev.tar` |
| `./tools/devcontainer-build.sh test-png` | Generate PNG snapshots | `dist/*.png` |
| `./tools/devcontainer-build.sh test-video` | Generate test videos | `test/output/*.mp4` |
| `./tools/devcontainer-build.sh shell` | Interactive shell in container | - |

#### Full Command Reference

```bash
# Container management
./tools/devcontainer-build.sh build      # Build container image
./tools/devcontainer-build.sh shell      # Interactive bash shell
./tools/devcontainer-build.sh exec "cmd" # Run arbitrary command

# WASM builds
./tools/devcontainer-build.sh wasm       # 4 WASM (production, ~640KB each)
./tools/devcontainer-build.sh wasm-debug # 4 WASM (debug, ~2.9MB each)
./tools/devcontainer-build.sh wasm-all   # 8 WASM (both modes)

# Packages (WASM + config + resources, signed)
./tools/devcontainer-build.sh package     # 4 production packages
./tools/devcontainer-build.sh package-dev # 4 debug packages

# Visual debugging
./tools/devcontainer-build.sh test-png    # PNG snapshots (4 variants)
./tools/devcontainer-build.sh test-video  # Test videos (4 variants)

# Protobuf
./tools/devcontainer-build.sh proto       # Update proto submodules

# CI / Full pipelines
./tools/devcontainer-build.sh ci          # Full CI (8 WASM + PNGs + videos)
./tools/devcontainer-build.sh all         # Container + WASM + PNGs
./tools/devcontainer-build.sh full        # Container + WASM + PNGs + videos
```

### CLion / IDE with Dev Container

CLion (and VS Code) can open the project directly inside the Dev Container for a fully integrated development experience.

#### Setup

1. **Open in CLion**: File → Remote Development → Dev Containers → Open existing container
2. **Select project root**: The `.devcontainer/devcontainer.json` configures the environment
3. **Wait for container build**: First run builds the toolchain image (~2-3 min)

#### Make Targets (Inside Container)

Once inside the container (via IDE terminal or `./tools/devcontainer-build.sh shell`), use Make directly:

```bash
# Default build
make                      # Build recording_day (production)
make BUILD_MODE=dev       # Build recording_day (debug)

# Build all variants
make all                  # 4 variants (production)
make all BUILD_MODE=dev   # 4 variants (debug)

# Packages
make package              # Build + sign recording_day
make package-all          # Build + sign all 4 variants
make package-dev          # Build + sign recording_day (debug)
make package-all-dev      # Build + sign all 4 variants (debug)

# Visual debugging
make png                  # PNG snapshot (recording_day)
make png-all              # PNG snapshots (4 variants)
make video                # Test videos (4 variants)

# CI
make ci                   # Full CI pipeline

# Utilities
make clean                # Remove all build artifacts
make help                 # Show all targets
```

#### Toolchain Versions

The Dev Container includes:
- **WASI SDK 20** - WebAssembly compilation
- **Wasmtime v27.0.0** - WASM runtime + C API
- **GStreamer 1.0** - Video generation
- **Node.js 20** - TypeScript tools
- **clang-format / clang-tidy** - Code quality (runs automatically)

---

## Visual Debugging

Generate PNG snapshots and test videos to verify OSD rendering without running the full application.

### PNG Snapshots

Static frame renders for quick visual inspection:

```bash
./tools/devcontainer-build.sh test-png
```

**Output**: `dist/*.png` (one per variant)

| File | Resolution | Description |
|------|------------|-------------|
| `live_day.png` | 1920×1080 | Day camera, no timestamp |
| `live_thermal.png` | 900×720 | Thermal camera, no timestamp |
| `recording_day.png` | 1920×1080 | Day camera, with timestamp |
| `recording_thermal.png` | 900×720 | Thermal camera, with timestamp |

### Test Videos

Animated sequences showing navball rotation, speed indicators, and timestamp updates:

```bash
./tools/devcontainer-build.sh test-video
```

**Output**: `test/output/*.mp4` (~192MB each, 30fps H.264)

Videos are useful for:
- Verifying navball rotation smoothness
- Testing speed indicator animations
- Checking timestamp formatting
- Validating celestial body positioning

---

## Build Artifacts

### Output Directories

```
build/              WASM binaries and intermediate objects
├── *.wasm          Production builds (~640KB each)
├── *_dev.wasm      Debug builds (~2.9MB each)
└── png_harness     Native test harness

dist/               Release artifacts
├── *.tar           Signed production packages (~960KB each)
├── *-dev.tar       Signed debug packages (~1.7MB each)
└── *.png           PNG snapshots

test/output/        Generated test videos
└── *.mp4           H.264 videos (~192MB each)

logs/               Build logs (gitignored)
```

### Package Contents

Each `.tar` package contains:

```
jettison-osd-recording_day-1.0.0.tar
├── manifest.jwt              # RS256-signed metadata (version, checksums)
└── recording_day.tar.gz      # Inner archive
    ├── recording_day.wasm    # Compiled OSD module
    ├── recording_day.json    # Runtime configuration
    └── resources/            # Fonts, navball skins, indicators
```

### Build Modes

| Mode | Flags | WASM Size | Package Size | Features |
|------|-------|-----------|--------------|----------|
| **production** | `-Oz -flto -DNDEBUG` | ~640KB | ~960KB | Optimized, LOG_DEBUG/INFO disabled |
| **dev** | `-O1 -g -D_FORTIFY_SOURCE=2` | ~2.9MB | ~1.7MB | Debug symbols, all logging |

---

## Project Structure

```
.
├── src/                    # C source code
│   ├── osd_plugin.{c,h}    # Main renderer, WASM exports
│   ├── osd_state.{c,h}     # State accessors (hides protobuf)
│   ├── config_json.{c,h}   # JSON configuration parser
│   ├── core/               # Framebuffer, context management
│   ├── rendering/          # Text, blending, primitives
│   ├── widgets/            # Crosshair, navball, timestamp, celestial
│   └── utils/              # Math, resource lookup
│
├── proto/                  # Protobuf bindings (git submodules)
│   ├── c/                  # C/nanopb bindings
│   └── ts/                 # TypeScript bindings
│
├── resources/              # Runtime assets
│   ├── *.json              # Variant configurations (see note below)
│   ├── fonts/              # TrueType fonts
│   ├── navball_skins/      # Equirectangular textures
│   └── navball_indicators/ # SVG overlays
│
├── vendor/                 # Third-party libraries
│   ├── cJSON.{c,h}         # JSON parser
│   ├── stb_truetype.h      # Font rendering
│   ├── stb_image.h         # PNG loading
│   ├── nanosvg.h           # SVG rasterization
│   └── astronomy.{c,h}     # Celestial calculations
│
├── test/                   # Test harnesses
│   ├── osd_test.c          # PNG harness (Wasmtime C API)
│   └── video_harness/      # Video generation (GStreamer)
│
├── tools/                  # Build scripts
│   ├── devcontainer-build.sh  # Host build entry point
│   ├── build.sh            # WASM compilation
│   ├── package.sh          # Package + sign
│   ├── format.sh           # clang-format
│   └── lint.sh             # clang-tidy
│
├── .devcontainer/          # Dev Container config
│   ├── devcontainer.json   # Container settings
│   └── Dockerfile          # Toolchain image
│
├── keys/                   # Signing keys (demo only)
├── Makefile                # Build targets (container use)
├── CLAUDE.md               # Comprehensive technical docs
└── README.md               # This file
```

---

## Variants

Four compile-time variants for different use cases:

| Variant | Resolution | Timestamp | Use Case |
|---------|-----------|-----------|----------|
| `live_day` | 1920×1080 | Hidden | Live UI overlay, daylight camera |
| `live_thermal` | 900×720 | Hidden | Live UI overlay, thermal camera |
| `recording_day` | 1920×1080 | Visible | Video encoding, daylight |
| `recording_thermal` | 900×720 | Visible | Video encoding, thermal |

**Compile-time defines**:
- Mode: `-DOSD_MODE_LIVE` or `-DOSD_MODE_RECORDING`
- Stream: `-DOSD_STREAM_DAY` or `-DOSD_STREAM_THERMAL`

### Configuration Files

Each variant has a JSON config in `resources/{variant}.json`.

**⚠️ Required Fields for Frontend Compatibility**:

The frontend (prototype_basic) validates configs against a JSON schema. These fields are **required** even if the C code doesn't use them:

| Field | Location | Purpose |
|-------|----------|---------|
| `version` | Top-level | Semantic version (e.g., "1.0.0") - required by schema |
| `font` | `timestamp`, `speed_indicators`, `variant_info` | Per-widget font selection |

**DO NOT remove these fields** or OSD initialization will fail silently in the browser.

---

## Deployment Configuration

Configure deployment in `.env`:

```bash
DEPLOY_HOST=sych.local
DEPLOY_USER=archer
```

Deploy to sych.local (packages served by nginx at `/osd/`):

```bash
./tools/devcontainer-build.sh deploy       # Dev builds
./tools/devcontainer-build.sh deploy-prod  # Production builds
```

**Note:** Host's `/etc/hosts` is mounted into devcontainer for hostname resolution.

---

## Integration with prototype_basic

Deploy OSD packages to the video player:

```bash
# 1. Build package
./tools/devcontainer-build.sh package-dev

# 2. Copy to prototype_basic
cp dist/jettison-osd-recording_day-1.0.0-dev.tar \
   /path/to/prototype_basic/web/public/osd/default.tar

# 3. Deploy player
cd /path/to/prototype_basic && make deploy
```

**Note**: Only `recording_day` variant is supported for video playback. Users may need to clear browser storage after package updates.

---

## Technical Details

See [`CLAUDE.md`](CLAUDE.md) for comprehensive documentation:

- Widget system architecture
- Protobuf integration (JonGUIState message)
- WASM API exports
- Configuration schema
- Packaging and signing format
- Quality gates (format, lint)
