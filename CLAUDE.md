# Claude AI Assistant Context - Jettison WASM OSD

## Project Overview

C-to-WebAssembly OSD (On-Screen Display) plugin for Jettison. Compiles to WASM using WASI SDK and renders telemetry overlays including crosshair, navball, timestamp, and speed indicators.

**Key Features**:
- Docker-first development (Dev Container with full toolchain)
- Make-based build system (CLion compatible)
- 4 WASM variants (live/recording x day/thermal)
- 2 build modes (production/debug)
- Cryptographic package signing (RSA-SHA256)

## Claude Code Integration

This project includes Claude Code skills and slash commands for streamlined development.

### Skills

Skills are auto-triggered based on context. Available in `.claude/skills/`:

| Skill | Description |
|-------|-------------|
| `build-helper` | WASM compilation, build modes, packaging |
| `deployment` | Deploy to sych.local, hot-reload workflow |
| `config-management` | JSON config editing, schema, widget settings |
| `protobuf` | Proto submodules, JonGUIState message structure |

### Slash Commands

Quick actions available via `/command`:

| Command | Description |
|---------|-------------|
| `/build` | Build all 4 WASM variants |
| `/deploy` | Build and deploy to sych.local |
| `/test` | Generate PNG snapshots for all variants |
| `/ci` | Run full CI pipeline (8 WASM + PNGs + videos) |
| `/proto` | Update protobuf submodules |
| `/shell` | Open interactive container shell |

## Build System

**Unified Interface**: Use `make` from anywhere (host or devcontainer). On the host, `make` automatically delegates to Docker via `./tools/devcontainer-build.sh`.

### Prerequisites

```bash
# Required on host machine:
# 1. Docker (must be running)
# 2. @devcontainers/cli (auto-installed if missing, or: npm install -g @devcontainers/cli)
```

### Quick Start

```bash
# Build all 4 production WASM variants
make all

# Build all 4 dev WASM variants (debug symbols)
make all-dev

# Build + sign all 4 production packages (~960KB each)
make package-all

# Build + sign all 4 dev packages (~1.7MB each)
make package-all-dev

# Build and deploy to sych.local
make deploy           # dev builds
make deploy-prod      # production builds

# Generate PNG snapshots for all 4 variants
make png-all

# Full CI pipeline (8 WASM + PNGs + videos)
make ci
```

### All Available Make Targets

| Target | Description | Output Location |
|--------|-------------|-----------------|
| `make all` | Build 4 WASM (production) | `build/*.wasm` |
| `make all-dev` | Build 4 WASM (debug) | `build/*_dev.wasm` |
| `make all-modes` | Build 8 WASM (both modes) | `build/*.wasm` |
| `make package-all` | 4 signed packages (production) | `dist/*.tar` |
| `make package-all-dev` | 4 signed packages (debug) | `dist/*-dev.tar` |
| `make deploy` | Build + deploy (dev) | sych.local |
| `make deploy-prod` | Build + deploy (production) | sych.local |
| `make png-all` | Generate PNG snapshots | `dist/*.png` |
| `make video` | Generate test videos | `test/output/*.mp4` |
| `make proto` | Update proto submodules | `proto/c/`, `proto/ts/` |
| `make ci` | Full CI pipeline | all outputs |
| `make help` | Show all available targets | - |

### Additional Host Commands

The devcontainer wrapper provides extra utilities:

```bash
./tools/devcontainer-build.sh shell      # Interactive container shell
./tools/devcontainer-build.sh exec "cmd" # Run arbitrary command in container
./tools/devcontainer-build.sh build      # Rebuild container image
```

### IDE Support (CLion)

Inside the container, additional targets for single-variant builds:

```bash
make                    # Build recording_day (default)
make recording_day      # Build single variant
make index              # Generate compile_commands.json for CLion/clangd
```

The project includes a pre-generated `compile_commands.json` for CLion code intelligence. This file is committed to the repo since dev container paths are stable (`/workspaces/dev_cont_test/...`).

To regenerate after adding new source files:
```bash
./tools/devcontainer-build.sh exec "make index"
```

This runs `bear` on all 4 variants and merges the results with `jq`, capturing all `-D` define combinations so CLion sees all code paths.

### Build Modes

| Mode | Flags | WASM Size | Package Size | Features |
|------|-------|-----------|--------------|----------|
| production | `-Oz -flto -DNDEBUG` | ~640 KB | ~960 KB | Optimized, no debug, no LOG_DEBUG/INFO |
| dev | `-O1 -g -D_FORTIFY_SOURCE=2` | ~2.9 MB | ~1.7 MB | Debug symbols, sanitizers, all logging |

**Logging behavior**:
- Production: `LOG_DEBUG()` and `LOG_INFO()` compile to nothing (like `assert()`)
- Development: All log levels active (`LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, `LOG_ERROR`)
- `LOG_WARN()` and `LOG_ERROR()` always active in both modes

**Dead code detection**: Build warns on unused functions/variables in source files (not vendor)

## 4-Variant Architecture

| Variant | Resolution | Timestamp | Navball | Crosshair |
|---------|-----------|-----------|---------|-----------|
| live_day | 1920x1080 | Hidden | Disabled | Vertical (+) |
| live_thermal | 900x720 | Hidden | Disabled | Diagonal (X) |
| recording_day | 1920x1080 | Visible | 5thHorseman_brown | Vertical (+) |
| recording_thermal | 900x720 | Visible | 5thHorseman_v2 | Diagonal (X) |

**Live variant optimization**: Live variants disable navball and celestial indicators for minimal rendering cost (crosshair + variant_info only).

**Compile-time defines**:
- `-DOSD_MODE_LIVE` or `-DOSD_MODE_RECORDING`
- `-DOSD_STREAM_DAY` or `-DOSD_STREAM_THERMAL`

## Widget System

All widgets implement: `bool widget_render(osd_context_t *ctx, osd_state_t *state)`

Returns `true` if something was rendered (enables GPU upload skipping when static).

Note: `variant_info_render()` uses `const osd_state_t *state` since it doesn't modify state.

### Crosshair (`src/widgets/crosshair.c`)
- Primitive rendering (center dot, cross, circle)
- Center offset from protobuf (`rec_osd.day_crosshair_offset_*` or `rec_osd.heat_crosshair_offset_*`)
- Speed indicators with configurable display:
  - Shows only when `rotary.is_moving == true` AND normalized speed > threshold
  - Threshold filtering: proto speeds are already normalized (-1.0 to 1.0), show if `|speed| > threshold` (default 0.05)
  - Display conversion: `degrees = normalized_speed * max_speed` (e.g., 0.5 * 35 = 17.5°)
  - Radial positioning: 110px horizontal, 90px vertical from center
  - Format: 3 decimal places with 1px black outline
  - Config: `speed_indicators.threshold` (0.0-1.0), `max_speed_azimuth/elevation` (default 35.0)

### Navball (`src/widgets/navball.c`)
- KSP-style orientation indicator
- Equirectangular projection + quaternion rotation (cglm)
- Rotation order: YXZ intrinsic (pitch -> roll -> yaw)
- 13 available skins via registry lookup
- **LUT optimization**: Precomputes ~51,000 sphere geometry entries, eliminating ~70,000 sqrt calls per frame
- Level marker: Optional horizontal white line at center
- Center indicator: Configurable SVG overlay (circle, rectangle, or crosshair)
- Simple diffuse lighting: `0.4 + 0.6 * (N·L)` with light at (0.3, 0.3, 1.0)

### Celestial Indicators (`src/widgets/navball.c`)
- Sun/moon position tracking on navball using astronomy calculations
- Uses `actual_space_time` message for observer location and timestamp
- Front hemisphere (visible): Full size, full opacity
- Back hemisphere (below horizon): 70% size, 50% opacity
- Configurable visibility threshold (e.g., -5.0° shows bodies slightly below horizon)
- 4 SVG variants per body: front, back, circle_front, circle_back

### Timestamp (`src/widgets/timestamp.c`)
- UTC time from protobuf (`time.timestamp`)
- Hidden in live builds, visible in recording builds
- Format: `HH:MM:SS UTC` with 2px black outline
- Configurable position, color, and font size

### Variant Info (`src/widgets/variant_info.c`)
- Debug overlay showing variant name and config values
- Displays: draw count, state time, frame delta, resolution, mode, and widget enable states
- Multi-line text with 1px outline, 4px line spacing
- **Draw Count**: Increments each state update, useful for verifying render pipeline
- **Frame Delta**: Shows latency between frame capture time and state time in milliseconds (variant-specific: Day or Heat)
- **Always returns true**: When enabled, forces texture re-upload every frame (draw count changes each render)

### ROI Overlay (`src/widgets/roi.c`)
- Colored bounding box overlays for active ROI (Region of Interest) regions
- 4 ROI types with distinct colors: FOCUS (green), TRACK (yellow), ZOOM (magenta), FX (cyan)
- Data source: `JonGuiDataCV` proto state fields (NOT opaque payloads like detections)
- State accessor: `osd_state_get_rois()` reads `cv.roi_focus_day` / `cv.roi_focus_heat` etc.
- Stream-aware via compile-time `OSD_STREAM_THERMAL` define (selects day vs heat ROIs)
- NDC coordinates (-1.0 to 1.0) converted to pixel space: `px = (ndc + 1.0) / 2.0 * width`
- Each ROI drawn as rectangle outline + labeled tag with semi-transparent background
- Only renders when ROI data is present in state (no visual noise otherwise)
- Disabled in PiP views via `pip_override.json`
- Config: `roi.enabled`, `roi.box_thickness`, `roi.label_font_size`, `roi.color_focus/track/zoom/fx`

## Protobuf Integration

**Git Submodules** (auto-updated by protogen CI):
- `proto/c/` → [jettison_proto_c](https://github.com/lpportorino/jettison_proto_c) - C nanopb bindings
- `proto/ts/` → [jettison_proto_typescript](https://github.com/lpportorino/jettison_proto_typescript) - TypeScript bindings (ts-proto)

**Update protos**: `make proto` or `./tools/devcontainer-build.sh proto` (updates submodules to latest + syncs to src/proto)

**Key message**: `JonGUIState` containing:
- `actual_space_time` - **Primary orientation source** for navball: azimuth, elevation, bank, plus observer location (lat/lon/alt) and timestamp for celestial calculations
- `compass` - Alternative orientation (azimuth 0-360, elevation -90 to 90, bank -180 to 180)
- `rotary` - Speed indicators: azimuth_speed, elevation_speed (normalized -1.0 to 1.0), is_moving flag
- `time` - Timestamp display (Unix epoch)
- `rec_osd` - Crosshair offsets: day_crosshair_offset_*, heat_crosshair_offset_*
- `cv` - ROI overlays: roi_focus_day/heat, roi_track_day/heat, roi_zoom_day/heat, roi_fx_day/heat (NDC coords, tags 40-53)

## Configuration (JSON)

Each variant has a config file: `resources/{variant}.json`

**IMPORTANT - Required Fields for Frontend Compatibility**:
The frontend (prototype_basic) validates configs against a JSON schema. These fields are **required**:
- `version` - Semantic version string (e.g., "1.0.0"). **DO NOT REMOVE** - required by frontend schema validation even though the C code doesn't use it.
- Per-widget `font` fields in `timestamp`, `speed_indicators`, and `variant_info` sections.

Key settings:
- `crosshair.orientation` - "vertical" or "diagonal"
- `timestamp.enabled` - true (recording) or false (live)
- `navball.skin` - One of 13 registered skins
- `navball.center_indicator.indicator` - "circle", "rectangle", or "crosshair"
- `speed_indicators.threshold` - Min normalized speed to show (0.0-1.0, default 0.05)
- `speed_indicators.max_speed_azimuth/elevation` - Display scale (default 35.0)
- `celestial_indicators.enabled` - Enable sun/moon tracking
- `celestial_indicators.visibility_threshold` - Altitude threshold for display (e.g., -5.0)
- `celestial_indicators.scale` - Size multiplier for indicators

**Font options** (available for `timestamp.font`, `speed_indicators.font`, `variant_info.font`):
- `liberation_sans_bold` - Default, clean sans-serif
- `b612_mono_bold` - Monospace, designed for cockpit displays
- `share_tech_mono` - Technical monospace
- `orbitron_bold` - Futuristic display font

**Schema**: `resources/schemas/osd_config.schema.json`

**Proto-sourced values** (NOT in config, come from protobuf state at runtime):
- Navball orientation: `actual_space_time.azimuth` / `.elevation` / `.bank`
- Celestial positions: `actual_space_time.latitude` / `.longitude` / `.altitude` / `.timestamp`
- Speed values: `rotary.azimuth_speed` / `rotary.elevation_speed` / `rotary.is_moving`
- Crosshair offset: `rec_osd.day_crosshair_offset_*` / `rec_osd.heat_crosshair_offset_*`
- Timestamp: `time.timestamp`
- Frame timing: `system_monotonic_time_us` / `frame_monotonic_day_us` / `frame_monotonic_heat_us`
- ROI regions: `cv.roi_focus_day` / `cv.roi_track_day` / `cv.roi_zoom_day` / `cv.roi_fx_day` (and heat variants) — each has `{x1, y1, x2, y2}` in NDC

## Code Organization

```
src/
├── osd_plugin.{c,h}     # Main renderer, WASM exports
├── osd_state.{c,h}      # State accessors (hides protobuf)
├── config_json.{c,h}    # JSON configuration parser
├── core/                # Framebuffer, context
├── rendering/           # Text, blending, primitives
├── widgets/             # Crosshair, navball, timestamp, celestial, ROI, detections
├── utils/               # Math, resource lookup, celestial position
└── proto/               # Protobuf (27 files + nanopb)

vendor/
├── cJSON.{c,h}          # JSON parser
├── stb_truetype.h       # Font rendering
├── stb_image.h          # PNG loading
├── nanosvg.h            # SVG rasterization
└── astronomy.{c,h}      # Celestial calculations

test/
├── osd_test.c           # PNG harness (Wasmtime C API, includes benchmarking)
├── video_harness/       # Video generation (GStreamer + Wasmtime)
│   ├── main.c           # Driver with 6 animation types
│   ├── synthetic_state.c # Animated state generator
│   └── gst_pipeline.c   # H.264 encoding pipeline
└── *.json               # Test orientation scenarios
```

## WASM API

```c
// Initialize OSD context (must be called first)
// Returns: 0 on success, non-zero on error
int wasm_osd_init(void);

// Update state from protobuf-encoded JonGUIState
// Returns: 0 on success, non-zero on error
int wasm_osd_update_state(uint32_t state_ptr, uint32_t state_size);

// Render OSD to framebuffer
// Returns: 1 if rendered, 0 if skipped (no changes)
int wasm_osd_render(void);

// Get framebuffer pointer (RGBA, width * height * 4 bytes)
uint32_t wasm_osd_get_framebuffer(void);

// Cleanup and free resources
int wasm_osd_destroy(void);
```

## Packaging

Packages are signed tarballs with JWT manifest:

```
jettison-osd-recording_day-1.0.0.tar
├── manifest.jwt              # RS256-signed metadata
└── recording_day.tar.gz      # WASM + config + resources
```

### Inner Archive Contents

The inner `.tar.gz` contains these files with **exact names**:

| File | Required | Description |
|------|----------|-------------|
| `{variant}.wasm` | Yes | Compiled WASM binary (e.g., `recording_day.wasm`) |
| `config.json` | Yes | Runtime configuration values |
| `config.schema.json` | Yes | JSON Schema for config validation and form generation |
| `resources/` | Yes | Fonts, navball skins, SVG indicators |

**⚠️ CRITICAL - File Naming**:
- Schema file MUST be named `config.schema.json` (NOT `schema.json`)
- Config file MUST be named `config.json`
- These names are used by frontend, gallery, and OSD config editor

**Schema source**: `resources/schemas/osd_config.schema.json` → copied as `config.schema.json`

**Manifest contains**: Version, git SHA, SHA256 checksums, system requirements.

**Keys**: `keys/example-{private,public}.pem` (demo only - generate unique keys for production)

## Dev Container

**Toolchain** (`.devcontainer/Dockerfile`):
- Ubuntu 22.04
- WASI SDK 29 (LLVM 21.1)
- Wasmtime v41.0.1 + C API
- GStreamer 1.0
- Node.js 20 + TypeScript
- clang-format + clang-tidy

**Entry points**:
- CLion: File -> Remote Development -> Dev Containers
- CLI: `./tools/devcontainer-build.sh shell`

**CLion Git Config Fix**:
CLion 2025.3+ may hang at "Setting global Git config" during dev container setup. This is mitigated by:
1. Host `~/.gitconfig` mounted to container via `devcontainer.json`
2. Fallback git config pre-set in Dockerfile
3. Pre-created `/.jbdevcontainer/config/JetBrains` directory

**Submodule Portability**:
The `.git/modules/` mount uses dynamic resolution via `initializeCommand` (runs `git rev-parse --git-dir` and creates a symlink at `/tmp/dc-gitmod/dev_cont_test`). This works regardless of nesting depth — whether dev_cont_test is a direct submodule of meta_web or a sub-sub-module of jettison. **Never hardcode `.git/modules/` paths** in devcontainer.json mounts.

**Docker BuildKit Note**:
If using a custom buildx builder with `docker-container` driver, switch to default for local builds:
```bash
docker buildx use default
```

## Quality Gates

Every build runs automatically:
1. `clang-format` - GNU Coding Standard
2. `clang-tidy` - Warnings as errors

**Exclusions**: `vendor/`, `proto/`, `svg.c`, `config_json.c`, `resource_lookup.c`

## Output Directories

```
build/       # WASM binaries (~640KB production, ~2.9MB debug)
dist/        # Signed packages (.tar) and PNG snapshots (.png)
test/output/ # Generated test videos (.mp4)
logs/        # Build logs
snapshot/    # PNG harness output (gitignored)
```

## Deployment

Packages deploy directly to sych.local where nginx serves them. No local paths - rsync over SSH.

### Configuration (.env)

```bash
# Remote deployment (SSH to sych.local)
DEPLOY_HOST=sych.local
DEPLOY_USER=archer
```

### Deploy Targets

| Target | Command | Description |
|--------|---------|-------------|
| `make deploy` | Dev build | Package + deploy all to sych.local |
| `make deploy-prod` | Production build | Package + deploy all (optimized) |
| `make deploy-frontend` | Dev build | Deploy live_day + live_thermal only |
| `make deploy-gallery` | Dev build | Deploy recording_day only |

Or via devcontainer wrapper:
```bash
./tools/devcontainer-build.sh deploy       # Dev builds
./tools/devcontainer-build.sh deploy-prod  # Production builds
```

### What Gets Deployed

**To sych.local:/home/archer/web/osd/ (centralized OSD packages):**
```
live_day.tar              # For jettison_frontend live streams
live_thermal.tar
pip_override.json         # PiP view config overrides
default.tar               # Gallery loads this on startup (recording_day)
```

### PiP Override Configuration

The `resources/pip_override.json` file contains config overrides for Picture-in-Picture views. PiP views should show minimal OSD - only the crosshair is visible.

**Location:** `resources/pip_override.json`

**Current overrides:**
```json
{
  "variant_info": { "enabled": false },
  "speed_indicators": { "enabled": false },
  "timestamp": { "enabled": false },
  "navball": { "enabled": false },
  "celestial_indicators": { "enabled": false },
  "detections": { "enabled": false },
  "roi": { "enabled": false }
}
```

**How it works:**
1. Deploy script rsyncs `pip_override.json` to `/osd/` on sych.local
2. OSD worker detects `viewType === 'pip'` from worker config
3. Worker fetches `/osd/pip_override.json` and merges it on top of base config
4. Result: PiP views only show crosshair, main views show all enabled widgets

**Editing:** Modify `resources/pip_override.json` and run `make deploy` to apply changes.

### Hot-Reload

The frontend loads OSD from signed packages (`/osd/*.tar`) and receives change notifications via SSE. When a package changes:
1. Worker detects ETag/Last-Modified change
2. Worker sends `packageChanged` event
3. OSDManager terminates and recreates worker with new package
4. SharedArrayBuffer survives - seamless transition

**Testing hot-reload:**
1. Make a change to OSD source
2. Run `make deploy` (builds + deploys to sych.local)
3. Frontend automatically detects and reloads within 1 second

### Notes

- Dev packages (~1.7MB) include debug symbols; production (~960KB) is optimized
- Gallery only uses `recording_day` variant
- Frontend uses `live_day` + `live_thermal` for live streams

---

**Build System**: Make + bash scripts
**Last Updated**: 2026-01-13
