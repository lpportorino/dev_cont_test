// Blending and Color System
// Provides alpha blending and color utilities for OSD rendering
//
// COLOR FORMAT: RGBA in memory (WebGL2 / GStreamer compatible)
// ==============================================================
// Colors are stored as uint32_t in 0xAABBGGRR format, which maps to
// RGBA byte order in memory on little-endian systems (x86, ARM).
//
// Representation:
//   - As uint32_t: 0xAABBGGRR (little-endian encoding)
//   - In memory:   [RR, GG, BB, AA] (WebGL2 RGBA format)
//
// Channels (as uint32_t):
//   - AA (bits 24-31): Alpha (0x00 = transparent, 0xFF = opaque)
//   - BB (bits 16-23): Blue (0x00-0xFF)
//   - GG (bits 8-15):  Green (0x00-0xFF)
//   - RR (bits 0-7):   Red (0x00-0xFF)
//
// Example colors:
//   - 0xFF0000FF: Opaque red   (memory: [FF, 00, 00, FF])
//   - 0xFF00FF00: Opaque green (memory: [00, FF, 00, FF])
//   - 0xFFFF0000: Opaque blue  (memory: [00, 00, FF, FF])
//   - 0x80FFFFFF: 50% transparent white
//   - 0x00000000: Fully transparent black

#ifndef RENDERING_BLENDING_H
#define RENDERING_BLENDING_H

#include <stdbool.h>
#include <stdint.h>

// ════════════════════════════════════════════════════════════
// COLOR CONSTRUCTION
// ════════════════════════════════════════════════════════════

// Construct RGBA color from individual components (0-255 each)
// Returns uint32_t in 0xAABBGGRR format (RGBA in memory)
//
// Usage:
//   uint32_t red = color_make_argb(255, 255, 0, 0);    // 0xFF0000FF
//   uint32_t semi_transparent_blue = color_make_argb(128, 0, 0, 255);  //
//   0x80FF0000
static inline uint32_t
color_make_argb(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
  // Format: 0xAABBGGRR (RGBA in memory on little-endian)
  return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8)
         | (uint32_t)r;
}

// ════════════════════════════════════════════════════════════
// COLOR COMPONENT EXTRACTION
// ════════════════════════════════════════════════════════════

// Extract alpha channel (0-255)
static inline uint8_t
color_get_alpha(uint32_t color)
{
  return (color >> 24) & 0xFF;
}

// Extract red channel (0-255)
static inline uint8_t
color_get_red(uint32_t color)
{
  return color & 0xFF;
}

// Extract green channel (0-255)
static inline uint8_t
color_get_green(uint32_t color)
{
  return (color >> 8) & 0xFF;
}

// Extract blue channel (0-255)
static inline uint8_t
color_get_blue(uint32_t color)
{
  return (color >> 16) & 0xFF;
}

// ════════════════════════════════════════════════════════════
// COLOR MANIPULATION
// ════════════════════════════════════════════════════════════

// Create new color with different alpha, keeping RGB unchanged
//
// Usage:
//   uint32_t opaque_red = 0xFF0000FF;
//   uint32_t semi_red = color_with_alpha(opaque_red, 128);  // 50% transparent
static inline uint32_t
color_with_alpha(uint32_t color, uint8_t new_alpha)
{
  return (color & 0x00FFFFFF) | ((uint32_t)new_alpha << 24);
}

// ════════════════════════════════════════════════════════════
// COLOR PARSING
// ════════════════════════════════════════════════════════════

// Parse color from hex string (web standard "#AARRGGBB" format)
//
// Accepts standard web color format and converts to internal RGBA (0xAABBGGRR).
//
// Supported formats:
//   - "#AARRGGBB": Full 8-digit hex (e.g., "#FFFF0000" = opaque red)
//   - "#RRGGBB":   6-digit hex, assumes full opacity (e.g., "#FF0000" = opaque
//   red)
//
// Returns 0xFFFFFFFF (opaque white) if parsing fails
//
// Usage:
//   uint32_t red = parse_color("#FFFF0000");   // Web format → internal RGBA
//   uint32_t blue = parse_color("#0000FF");    // Assumes alpha=FF
uint32_t parse_color(const char *hex);

// ════════════════════════════════════════════════════════════
// ALPHA BLENDING
// ════════════════════════════════════════════════════════════

// Alpha blend foreground color onto background color
//
// Uses Porter-Duff "over" compositing:
//   result = fg + bg * (1 - alpha_fg)
//
// Fast paths:
//   - If fg alpha = 0: returns bg unchanged (fully transparent)
//   - If fg alpha = 255: returns fg unchanged (fully opaque)
//
// Usage:
//   uint32_t bg = 0xFFFF0000;  // Blue background
//   uint32_t fg = 0x800000FF;  // 50% transparent red
//   uint32_t result = blend_argb(bg, fg);  // Purple-ish blend
uint32_t blend_argb(uint32_t bg, uint32_t fg);

// ════════════════════════════════════════════════════════════
// PREDEFINED COLORS
// ════════════════════════════════════════════════════════════

#define COLOR_TRANSPARENT 0x00000000
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_BLACK 0xFF000000
#define COLOR_RED 0xFF0000FF
#define COLOR_GREEN 0xFF00FF00
#define COLOR_BLUE 0xFFFF0000
#define COLOR_YELLOW 0xFF00FFFF
#define COLOR_CYAN 0xFFFFFF00
#define COLOR_MAGENTA 0xFFFF00FF

#endif // RENDERING_BLENDING_H
