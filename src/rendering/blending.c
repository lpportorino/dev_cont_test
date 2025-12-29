#include "blending.h"

#include <stdio.h>
#include <string.h>

// ════════════════════════════════════════════════════════════
// COLOR PARSING IMPLEMENTATION
// ════════════════════════════════════════════════════════════

/**
 * Parse hex color string to internal RGBA format
 *
 * Converts a hex color string to the internal RGBA uint32_t format used
 * throughout the rendering system. Handles both 6-digit (#RRGGBB) and
 * 8-digit (#AARRGGBB) formats.
 *
 * Input formats:
 *   - "#RRGGBB"   → RGB with full opacity (alpha = 0xFF)
 *   - "#AARRGGBB" → ARGB with specified alpha
 *
 * Output format:
 *   - Internal RGBA: 0xAABBGGRR (little-endian byte order)
 *   - Memory layout: [RR GG BB AA] (R at lowest address)
 *   - This matches GPU texture format for direct upload
 *
 * Examples:
 *   - "#FF0000"   → 0xFF0000FF (opaque red)
 *   - "#00FF00"   → 0xFF00FF00 (opaque green)
 *   - "#0000FF"   → 0xFFFF0000 (opaque blue)
 *   - "#80FF0000" → 0x800000FF (semi-transparent red, 50% alpha)
 *
 * @param hex Hex color string (must start with '#')
 * @return Internal RGBA uint32_t (0xAABBGGRR), or 0xFFFFFFFF (white) on error
 */
uint32_t
parse_color(const char *hex)
{
  // Validate input
  if (!hex || hex[0] != '#')
    {
      return 0xFFFFFFFF; // Default to opaque white
    }

  // Parse hex string to integer (AARRGGBB format)
  uint32_t color = 0;
  int parsed     = sscanf(hex + 1, "%x", &color);

  if (parsed != 1)
    {
      return 0xFFFFFFFF; // Default to opaque white
    }

  // Check if 6-digit format (no alpha specified)
  size_t len = strlen(hex + 1);
  if (len == 6)
    {
      // Assume full opacity: shift RGB and add alpha=FF
      color = 0xFF000000 | color;
    }

  // Convert from AARRGGBB (web format) to AABBGGRR (internal RGBA format)
  // Extract channels from web format
  uint32_t a = (color >> 24) & 0xFF;
  uint32_t r = (color >> 16) & 0xFF;
  uint32_t g = (color >> 8) & 0xFF;
  uint32_t b = color & 0xFF;

  // Reassemble in RGBA format: 0xAABBGGRR
  return (a << 24) | (b << 16) | (g << 8) | r;
}

// ════════════════════════════════════════════════════════════
// ALPHA BLENDING IMPLEMENTATION
// ════════════════════════════════════════════════════════════

uint32_t
blend_argb(uint32_t bg, uint32_t fg)
{
  // Extract foreground alpha
  uint32_t alpha = (fg >> 24) & 0xFF;

  // Fast path: fully transparent foreground
  if (alpha == 0)
    {
      return bg;
    }

  // Fast path: fully opaque foreground
  if (alpha == 255)
    {
      return fg;
    }

  // General case: Porter-Duff "over" compositing
  // result = fg + bg * (1 - alpha_fg)
  // result_alpha = alpha_fg + alpha_bg * (1 - alpha_fg)

  uint32_t bg_alpha  = (bg >> 24) & 0xFF;
  uint32_t inv_alpha = 255 - alpha;

  // Blend channels (RGBA format: 0xAABBGGRR)
  // Red channel (bits 0-7)
  uint32_t fg_r = fg & 0xFF;
  uint32_t bg_r = bg & 0xFF;
  uint32_t r    = ((fg_r * alpha) + (bg_r * inv_alpha)) / 255;

  // Green channel (bits 8-15)
  uint32_t fg_g = (fg >> 8) & 0xFF;
  uint32_t bg_g = (bg >> 8) & 0xFF;
  uint32_t g    = ((fg_g * alpha) + (bg_g * inv_alpha)) / 255;

  // Blue channel (bits 16-23)
  uint32_t fg_b = (fg >> 16) & 0xFF;
  uint32_t bg_b = (bg >> 16) & 0xFF;
  uint32_t b    = ((fg_b * alpha) + (bg_b * inv_alpha)) / 255;

  // Blend alpha channel (Porter-Duff "over")
  uint32_t result_alpha = alpha + ((bg_alpha * inv_alpha) / 255);

  // Combine channels in RGBA format (WebGL2/GStreamer compatible)
  // Memory layout: [R, G, B, A]
  // uint32_t value: 0xAABBGGRR
  return (result_alpha << 24) | (b << 16) | (g << 8) | r;
}
