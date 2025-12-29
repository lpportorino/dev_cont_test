#!/bin/bash
# Generate rotation video
# Creates N frames with varying rotations and combines into MP4

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Configuration
NUM_FRAMES=${1:-1000}
OUTPUT_DIR="snapshot/frames"
VIDEO_OUTPUT="snapshot/navball_rotation.mp4"
VIDEO_FPS=30

cd "$PROJECT_ROOT"

echo "========================================"
echo "  Nav Ball Rotation Video Generator"
echo "========================================"
echo "Frames: $NUM_FRAMES"
echo "Output: $VIDEO_OUTPUT"
echo ""

# Clean and create frames directory
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# Build the multi-frame test if needed
if [ ! -f "test/osd_test_multiframe" ]; then
  echo "Building multi-frame test harness..."
  ./test/build_multiframe_test.sh
fi

# Run multi-frame test to generate all PNGs
echo "Generating frames..."
./test/osd_test_multiframe "$NUM_FRAMES" "$OUTPUT_DIR"

echo ""
echo "Creating video with ffmpeg..."
# Optimize for quality and speed (not file size)
# - ultrafast: fastest encoding (file size doesn't matter)
# - crf 18: visually lossless quality (lower = better, 23 is default)
# - yuv420p: compatibility with most players
ffmpeg -y -framerate $VIDEO_FPS \
  -i "$OUTPUT_DIR/frame_%04d.png" \
  -c:v libx264 \
  -preset ultrafast \
  -crf 18 \
  -pix_fmt yuv420p \
  "$VIDEO_OUTPUT" \
  2>&1 | grep -E "frame=|video:|Duration:" || true

echo ""
echo "========================================"
echo "✅ Video created: $VIDEO_OUTPUT"
echo "========================================"

# Show video info
if command -v ffprobe &> /dev/null; then
  echo ""
  ffprobe -v error -select_streams v:0 \
    -show_entries stream=width,height,duration,nb_frames,r_frame_rate \
    -of default=noprint_wrappers=1 \
    "$VIDEO_OUTPUT" 2>/dev/null || true
fi

echo ""
echo "To view: ffplay $VIDEO_OUTPUT"
echo ""
