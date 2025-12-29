#include "primitives.h"

#include <math.h>
#include <stdlib.h>

// ════════════════════════════════════════════════════════════
// POINT DRAWING IMPLEMENTATION
// ════════════════════════════════════════════════════════════

void
draw_pixel(framebuffer_t *fb, int x, int y, uint32_t color)
{
  // Use framebuffer's bounds-checked blend function
  framebuffer_blend_pixel(fb, x, y, color);
}

// ════════════════════════════════════════════════════════════
// LINE DRAWING IMPLEMENTATION
// ════════════════════════════════════════════════════════════

void
draw_line(framebuffer_t *fb,
          int x0,
          int y0,
          int x1,
          int y1,
          uint32_t color,
          float thickness)
{
  // Bresenham's line algorithm with thickness
  int dx  = abs(x1 - x0);
  int dy  = abs(y1 - y0);
  int sx  = x0 < x1 ? 1 : -1;
  int sy  = y0 < y1 ? 1 : -1;
  int err = dx - dy;

  int half_thick = (int)(thickness / 2.0f);

  while (1)
    {
      // Draw thick point (square stamp perpendicular to line)
      for (int ty = -half_thick; ty <= half_thick; ty++)
        {
          for (int tx = -half_thick; tx <= half_thick; tx++)
            {
              draw_pixel(fb, x0 + tx, y0 + ty, color);
            }
        }

      // Check if we've reached the end point
      if (x0 == x1 && y0 == y1)
        {
          break;
        }

      // Bresenham's step
      int e2 = 2 * err;
      if (e2 > -dy)
        {
          err -= dy;
          x0 += sx;
        }
      if (e2 < dx)
        {
          err += dx;
          y0 += sy;
        }
    }
}

// ════════════════════════════════════════════════════════════
// CIRCLE DRAWING IMPLEMENTATION
// ════════════════════════════════════════════════════════════

void
draw_filled_circle(
  framebuffer_t *fb, int cx, int cy, float radius, uint32_t color)
{
  // Simple distance-based filling
  int r = (int)radius;

  for (int y = -r; y <= r; y++)
    {
      for (int x = -r; x <= r; x++)
        {
          // Check if point is within circle
          if (x * x + y * y <= r * r)
            {
              draw_pixel(fb, cx + x, cy + y, color);
            }
        }
    }
}

void
draw_circle_outline(framebuffer_t *fb,
                    int cx,
                    int cy,
                    float radius,
                    uint32_t color,
                    float thickness)
{
  // Midpoint circle algorithm with thickness
  // Draw all pixels between inner and outer radius
  int r_outer = (int)(radius + thickness / 2.0f);
  int r_inner = (int)(radius - thickness / 2.0f);

  if (r_inner < 0)
    {
      r_inner = 0;
    }

  for (int y = -r_outer; y <= r_outer; y++)
    {
      for (int x = -r_outer; x <= r_outer; x++)
        {
          int dist_sq = x * x + y * y;

          // Check if point is in the annular region (donut)
          if (dist_sq >= r_inner * r_inner && dist_sq <= r_outer * r_outer)
            {
              draw_pixel(fb, cx + x, cy + y, color);
            }
        }
    }
}

// ════════════════════════════════════════════════════════════
// RECTANGLE DRAWING IMPLEMENTATION
// ════════════════════════════════════════════════════════════

void
draw_rect_filled(framebuffer_t *fb, int x, int y, int w, int h, uint32_t color)
{
  // Draw all pixels in rectangle
  for (int py = y; py < y + h; py++)
    {
      for (int px = x; px < x + w; px++)
        {
          draw_pixel(fb, px, py, color);
        }
    }
}

void
draw_rect_outline(framebuffer_t *fb,
                  int x,
                  int y,
                  int w,
                  int h,
                  uint32_t color,
                  float thickness)
{
  // Draw four lines to form rectangle outline
  int t = (int)thickness;

  // Top edge
  draw_rect_filled(fb, x, y, w, t, color);

  // Bottom edge
  draw_rect_filled(fb, x, y + h - t, w, t, color);

  // Left edge
  draw_rect_filled(fb, x, y + t, t, h - 2 * t, color);

  // Right edge
  draw_rect_filled(fb, x + w - t, y + t, t, h - 2 * t, color);
}
