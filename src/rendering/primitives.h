// Primitive Drawing Functions
// Low-level geometric rendering primitives for OSD graphics
//
// All functions operate on a framebuffer and use alpha blending for smooth
// compositing. Coordinates are in pixels, with (0,0) at top-left.

#ifndef RENDERING_PRIMITIVES_H
#define RENDERING_PRIMITIVES_H

#include "../core/framebuffer.h"

#include <stdint.h>

// ════════════════════════════════════════════════════════════
// POINT DRAWING
// ════════════════════════════════════════════════════════════

// Draw single pixel with alpha blending
//
// Performs bounds checking and alpha blending automatically.
// If (x, y) is out of bounds, does nothing.
//
// Usage:
//   draw_pixel(&fb, 100, 100, 0xFF0000FF);  // Red pixel
void draw_pixel(framebuffer_t *fb, int x, int y, uint32_t color);

// ════════════════════════════════════════════════════════════
// LINE DRAWING
// ════════════════════════════════════════════════════════════

// Draw line from (x0, y0) to (x1, y1) with thickness
//
// Uses Bresenham's line algorithm with thickness support.
// Thick lines are drawn as squares perpendicular to the line.
//
// Parameters:
//   fb: Framebuffer to draw on
//   x0, y0: Start point
//   x1, y1: End point
//   color: RGBA color (0xAABBGGRR format)
//   thickness: Line width in pixels (can be fractional)
//
// Usage:
//   draw_line(&fb, 0, 0, 100, 100, 0xFF0000FF, 2.0f);  // Red diagonal line
void draw_line(framebuffer_t *fb,
               int x0,
               int y0,
               int x1,
               int y1,
               uint32_t color,
               float thickness);

// ════════════════════════════════════════════════════════════
// CIRCLE DRAWING
// ════════════════════════════════════════════════════════════

// Draw filled circle centered at (cx, cy) with given radius
//
// Uses simple distance check: draws all pixels within radius.
//
// Parameters:
//   fb: Framebuffer to draw on
//   cx, cy: Center point
//   radius: Circle radius in pixels (can be fractional)
//   color: RGBA color (0xAABBGGRR format)
//
// Usage:
//   draw_filled_circle(&fb, 100, 100, 10.0f, 0xFF0000FF);  // Red circle
void draw_filled_circle(
  framebuffer_t *fb, int cx, int cy, float radius, uint32_t color);

// Draw circle outline (hollow circle) with thickness
//
// Uses midpoint circle algorithm with thickness.
// Draws all pixels between inner and outer radius.
//
// Parameters:
//   fb: Framebuffer to draw on
//   cx, cy: Center point
//   radius: Circle radius in pixels (center of stroke)
//   color: RGBA color (0xAABBGGRR format)
//   thickness: Stroke width in pixels (can be fractional)
//
// Usage:
//   draw_circle_outline(&fb, 100, 100, 10.0f, 0xFF0000FF, 2.0f);
void draw_circle_outline(framebuffer_t *fb,
                         int cx,
                         int cy,
                         float radius,
                         uint32_t color,
                         float thickness);

// ════════════════════════════════════════════════════════════
// RECTANGLE DRAWING
// ════════════════════════════════════════════════════════════

// Draw filled rectangle with top-left at (x, y)
//
// Parameters:
//   fb: Framebuffer to draw on
//   x, y: Top-left corner
//   w, h: Width and height in pixels
//   color: RGBA color (0xAABBGGRR format)
//
// Usage:
//   draw_rect_filled(&fb, 10, 10, 100, 50, 0xFF0000FF);  // Red rectangle
void
draw_rect_filled(framebuffer_t *fb, int x, int y, int w, int h, uint32_t color);

// Draw rectangle outline with thickness
//
// Parameters:
//   fb: Framebuffer to draw on
//   x, y: Top-left corner
//   w, h: Width and height in pixels
//   color: RGBA color (0xAABBGGRR format)
//   thickness: Stroke width in pixels (can be fractional)
//
// Usage:
//   draw_rect_outline(&fb, 10, 10, 100, 50, 0xFF0000FF, 2.0f);
void draw_rect_outline(framebuffer_t *fb,
                       int x,
                       int y,
                       int w,
                       int h,
                       uint32_t color,
                       float thickness);

#endif // RENDERING_PRIMITIVES_H
