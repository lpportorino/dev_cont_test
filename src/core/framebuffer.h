// Framebuffer Management
// Provides safe access to the pixel buffer with bounds checking
//
// COLOR FORMAT: RGBA (WebGL2 / GStreamer compatible)
// ===================================================
// The framebuffer uses RGBA byte order in memory, which is the standard
// format for WebGL2 textures and GStreamer video overlays.
//
// Memory layout per pixel (4 bytes):
//   [Red, Green, Blue, Alpha]
//
// When accessed as uint32_t on little-endian (x86/ARM):
//   0xAABBGGRR (alpha in high byte, red in low byte)
//
// Example colors (as uint32_t):
//   Opaque Red:     0xFF0000FF  (memory: [FF, 00, 00, FF])
//   Opaque Green:   0xFF00FF00  (memory: [00, FF, 00, FF])
//   Opaque Blue:    0xFFFF0000  (memory: [00, 00, FF, FF])
//   Transparent:    0x00000000  (memory: [00, 00, 00, 00])
//
// 2D Array layout (row-major order):
//   [Row 0: pixel(0,0), pixel(1,0), pixel(2,0), ... pixel(width-1, 0)]
//   [Row 1: pixel(0,1), pixel(1,1), pixel(2,1), ... pixel(width-1, 1)]
//   ...
//   [Row height-1: pixel(0,height-1), ... pixel(width-1, height-1)]

#ifndef CORE_FRAMEBUFFER_H
#define CORE_FRAMEBUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ════════════════════════════════════════════════════════════
// FRAMEBUFFER STRUCTURE
// ════════════════════════════════════════════════════════════

typedef struct
{
  uint32_t *data;  // Pixel buffer (ARGB format)
  uint32_t width;  // Width in pixels
  uint32_t height; // Height in pixels
  size_t stride;   // Bytes per row (usually width * 4)
} framebuffer_t;

// ════════════════════════════════════════════════════════════
// LIFECYCLE
// ════════════════════════════════════════════════════════════

// Initialize framebuffer structure (does not allocate memory)
//
// The caller must provide a pre-allocated pixel buffer.
//
// Usage:
//   uint32_t pixels[1920 * 1080];
//   framebuffer_t fb;
//   framebuffer_init(&fb, pixels, 1920, 1080);
void framebuffer_init(framebuffer_t *fb,
                      uint32_t *data,
                      uint32_t width,
                      uint32_t height);

// ════════════════════════════════════════════════════════════
// BUFFER OPERATIONS
// ════════════════════════════════════════════════════════════

// Clear framebuffer to solid color
//
// Usage:
//   framebuffer_clear(&fb, 0x00000000);  // Transparent
//   framebuffer_clear(&fb, 0xFF000000);  // Opaque black
void framebuffer_clear(framebuffer_t *fb, uint32_t color);

// ════════════════════════════════════════════════════════════
// PIXEL ACCESS
// ════════════════════════════════════════════════════════════

// Check if coordinates are within bounds
//
// Returns true if (x, y) is valid, false otherwise
static inline bool
framebuffer_in_bounds(const framebuffer_t *fb, int x, int y)
{
  return (x >= 0 && x < (int)fb->width && y >= 0 && y < (int)fb->height);
}

// Get pixel color at (x, y) - safe with bounds checking
//
// Returns color at (x, y), or 0x00000000 if out of bounds
uint32_t framebuffer_get_pixel(const framebuffer_t *fb, int x, int y);

// Set pixel color at (x, y) - safe with bounds checking
//
// Does nothing if (x, y) is out of bounds
void framebuffer_set_pixel(framebuffer_t *fb, int x, int y, uint32_t color);

// Blend pixel color at (x, y) using alpha compositing
//
// Performs: fb[x,y] = blend_argb(fb[x,y], color)
// Does nothing if (x, y) is out of bounds
//
// Usage:
//   // Draw semi-transparent red pixel
//   framebuffer_blend_pixel(&fb, 100, 100, 0x80FF0000);
void framebuffer_blend_pixel(framebuffer_t *fb, int x, int y, uint32_t color);

// ════════════════════════════════════════════════════════════
// DIRECT ACCESS (UNSAFE - USE WITH CAUTION)
// ════════════════════════════════════════════════════════════

// Get direct pointer to pixel at (x, y) - NO bounds checking
//
// WARNING: Caller MUST ensure (x, y) is in bounds!
// Use framebuffer_in_bounds() first if unsure.
//
// Usage:
//   if (framebuffer_in_bounds(&fb, x, y)) {
//     uint32_t *pixel = framebuffer_get_pixel_ptr(&fb, x, y);
//     *pixel = blend_argb(*pixel, color);
//   }
static inline uint32_t *
framebuffer_get_pixel_ptr(framebuffer_t *fb, int x, int y)
{
  return &fb->data[y * fb->width + x];
}

#endif // CORE_FRAMEBUFFER_H
