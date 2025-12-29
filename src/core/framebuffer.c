#include "framebuffer.h"

#include "../rendering/blending.h"

#include <string.h>

// ════════════════════════════════════════════════════════════
// LIFECYCLE IMPLEMENTATION
// ════════════════════════════════════════════════════════════

void
framebuffer_init(framebuffer_t *fb,
                 uint32_t *data,
                 uint32_t width,
                 uint32_t height)
{
  fb->data   = data;
  fb->width  = width;
  fb->height = height;
  fb->stride = width * sizeof(uint32_t);
}

// ════════════════════════════════════════════════════════════
// BUFFER OPERATIONS IMPLEMENTATION
// ════════════════════════════════════════════════════════════

void
framebuffer_clear(framebuffer_t *fb, uint32_t color)
{
  size_t num_pixels = (size_t)fb->width * (size_t)fb->height;

  // Fast path: use memset for common colors
  if (color == 0x00000000)
    {
      // Transparent black - zero out entire buffer
      memset(fb->data, 0, num_pixels * sizeof(uint32_t));
      return;
    }

  // General case: fill with color
  for (size_t i = 0; i < num_pixels; i++)
    {
      fb->data[i] = color;
    }
}

// ════════════════════════════════════════════════════════════
// PIXEL ACCESS IMPLEMENTATION
// ════════════════════════════════════════════════════════════

uint32_t
framebuffer_get_pixel(const framebuffer_t *fb, int x, int y)
{
  // Bounds check
  if (!framebuffer_in_bounds(fb, x, y))
    {
      return 0x00000000; // Return transparent black if out of bounds
    }

  // Calculate offset and return pixel
  uint32_t idx = y * fb->width + x;
  return fb->data[idx];
}

void
framebuffer_set_pixel(framebuffer_t *fb, int x, int y, uint32_t color)
{
  // Bounds check
  if (!framebuffer_in_bounds(fb, x, y))
    {
      return;
    }

  // Calculate offset and set pixel
  uint32_t idx  = y * fb->width + x;
  fb->data[idx] = color;
}

void
framebuffer_blend_pixel(framebuffer_t *fb, int x, int y, uint32_t color)
{
  // Bounds check
  if (!framebuffer_in_bounds(fb, x, y))
    {
      return;
    }

  // Calculate offset
  uint32_t idx = y * fb->width + x;

  // Alpha blend: result = blend_argb(background, foreground)
  fb->data[idx] = blend_argb(fb->data[idx], color);
}
