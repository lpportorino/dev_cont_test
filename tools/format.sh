#!/bin/bash
# Format all C source files using clang-format

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

echo "🎨 Formatting C source files..."
echo "Using GNU coding standard"
echo ""

# Find all .c and .h files in src/
FILES=$(find src -type f \( -name "*.c" -o -name "*.h" \) 2>/dev/null || true)

if [ -z "$FILES" ]; then
  echo "⚠️  No source files found to format"
  exit 0
fi

FORMATTED_COUNT=0

for file in $FILES; do
  echo "  Formatting: $file"
  clang-format -i -style=file "$file"
  FORMATTED_COUNT=$((FORMATTED_COUNT + 1))
done

echo ""
echo "✅ Formatted $FORMATTED_COUNT files"
echo ""
