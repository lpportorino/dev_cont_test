#!/bin/bash
set -e

echo "Compiling PNG test harness..."
cd test
gcc -I. -I../vendor -o osd_test osd_test.c -lwasmtime -lpng -lm
cd ..

echo ""
echo "Generating PNGs for all 4 variants..."
echo ""

# Run from /workspace so WASM sees correct resource paths
for variant in live_day live_thermal recording_day recording_thermal; do
  echo "=== Generating PNG: $variant ==="
  OUTPUT_PNG="snapshot/${variant}.png" ./test/osd_test "build/${variant}.wasm" || exit 1
  echo ""
done

echo "✅ All 4 PNGs generated successfully"
echo ""
ls -lh snapshot/*.png
