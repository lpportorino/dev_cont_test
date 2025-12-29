/**
 * @file utils.c
 * @brief OSD utility function implementations
 *
 * Reusable rendering utilities extracted for maintainability.
 */

#include "utils.h"

#include <stdlib.h>
#include <string.h>

/* ==================== Color Utilities ==================== */

uint32_t
color_to_u32(Color c)
{
  return (c.r << 16) | (c.g << 8) | c.b;
}

/* ==================== Rectangle Utilities ==================== */

void
clear_rect(
  uint8_t *fb, int32_t stride, int32_t x, int32_t y, int32_t w, int32_t h)
{
  for (int32_t row = y; row < y + h; row++)
    {
      memset(fb + row * stride + x * 4, 0, w * 4);
    }
}

/* ==================== Primitive Drawing ==================== */

void
draw_filled_rect(uint8_t *framebuffer,
                 int32_t stride,
                 int32_t width,
                 int32_t height,
                 int32_t x,
                 int32_t y,
                 int32_t w,
                 int32_t h,
                 Color c)
{
  for (int32_t row = y; row < y + h; row++)
    {
      if (row < 0 || row >= height)
        continue;
      for (int32_t col = x; col < x + w; col++)
        {
          if (col < 0 || col >= width)
            continue;
          uint8_t *pixel = framebuffer + row * stride + col * 4;
          pixel[0]       = c.r;
          pixel[1]       = c.g;
          pixel[2]       = c.b;
          pixel[3]       = c.a;
        }
    }
}

void
draw_filled_circle(uint8_t *framebuffer,
                   int32_t stride,
                   int32_t width,
                   int32_t height,
                   int32_t cx,
                   int32_t cy,
                   int32_t radius,
                   Color c)
{
  int32_t radius_sq = radius * radius;
  for (int32_t dy = -radius; dy <= radius; dy++)
    {
      int32_t y = cy + dy;
      if (y < 0 || y >= height)
        continue;
      for (int32_t dx = -radius; dx <= radius; dx++)
        {
          if (dx * dx + dy * dy > radius_sq)
            continue;
          int32_t x = cx + dx;
          if (x < 0 || x >= width)
            continue;
          uint8_t *pixel = framebuffer + y * stride + x * 4;
          pixel[0]       = c.r;
          pixel[1]       = c.g;
          pixel[2]       = c.b;
          pixel[3]       = c.a;
        }
    }
}

void
draw_circle_outline(uint8_t *framebuffer,
                    int32_t stride,
                    int32_t width,
                    int32_t height,
                    int32_t cx,
                    int32_t cy,
                    int32_t radius,
                    int32_t thickness,
                    Color c)
{
  int32_t outer_sq     = radius * radius;
  int32_t inner_radius = radius - thickness;
  int32_t inner_sq     = inner_radius * inner_radius;

  for (int32_t dy = -radius; dy <= radius; dy++)
    {
      int32_t y = cy + dy;
      if (y < 0 || y >= height)
        continue;
      for (int32_t dx = -radius; dx <= radius; dx++)
        {
          int32_t dist_sq = dx * dx + dy * dy;
          if (dist_sq < inner_sq || dist_sq > outer_sq)
            continue;
          int32_t x = cx + dx;
          if (x < 0 || x >= width)
            continue;
          uint8_t *pixel = framebuffer + y * stride + x * 4;
          pixel[0]       = c.r;
          pixel[1]       = c.g;
          pixel[2]       = c.b;
          pixel[3]       = c.a;
        }
    }
}

/* ==================== Text Rendering ==================== */

int32_t
measure_text_width(stbtt_fontinfo *font, const char *text, float size)
{
  if (!font || !text)
    return 0;

  float scale         = stbtt_ScaleForPixelHeight(font, size);
  int32_t total_width = 0;

  for (const char *p = text; *p; p++)
    {
      int advance, lsb;
      stbtt_GetCodepointHMetrics(font, *p, &advance, &lsb);
      total_width += (int32_t)(advance * scale);
    }

  return total_width;
}

void
render_text(uint8_t *framebuffer,
            int32_t stride,
            int32_t width,
            int32_t height,
            stbtt_fontinfo *font,
            const char *text,
            int32_t x,
            int32_t y,
            float size,
            uint32_t color)
{
  if (!font || !text)
    return;

  float scale = stbtt_ScaleForPixelHeight(font, size);
  int ascent, descent, line_gap;
  stbtt_GetFontVMetrics(font, &ascent, &descent, &line_gap);
  int baseline = (int)(ascent * scale);

  int32_t cursor_x = x;
  uint8_t r        = (color >> 16) & 0xFF;
  uint8_t g        = (color >> 8) & 0xFF;
  uint8_t b        = color & 0xFF;

  for (const char *p = text; *p; p++)
    {
      int advance, lsb;
      stbtt_GetCodepointHMetrics(font, *p, &advance, &lsb);

      int glyph_w, glyph_h, xoff, yoff;
      unsigned char *bitmap = stbtt_GetCodepointBitmap(
        font, scale, scale, *p, &glyph_w, &glyph_h, &xoff, &yoff);
      if (!bitmap)
        {
          cursor_x += (int)(advance * scale);
          continue;
        }

      int glyph_x = cursor_x + xoff;
      int glyph_y = y + baseline + yoff;

      for (int gy = 0; gy < glyph_h; gy++)
        {
          int fb_y = glyph_y + gy;
          if (fb_y < 0 || fb_y >= height)
            continue;
          for (int gx = 0; gx < glyph_w; gx++)
            {
              int fb_x = glyph_x + gx;
              if (fb_x < 0 || fb_x >= width)
                continue;

              uint8_t alpha = bitmap[gy * glyph_w + gx];
              if (alpha == 0)
                continue;

              uint8_t *dst       = framebuffer + fb_y * stride + fb_x * 4;
              uint32_t inv_alpha = 255 - alpha;
              dst[0]             = (r * alpha + dst[0] * inv_alpha) / 255;
              dst[1]             = (g * alpha + dst[1] * inv_alpha) / 255;
              dst[2]             = (b * alpha + dst[2] * inv_alpha) / 255;
              dst[3]             = 255;
            }
        }

      stbtt_FreeBitmap(bitmap, NULL);
      cursor_x += (int)(advance * scale);
    }
}

void
render_text_centered(uint8_t *framebuffer,
                     int32_t stride,
                     int32_t width,
                     int32_t height,
                     stbtt_fontinfo *font,
                     const char *text,
                     int32_t box_x,
                     int32_t box_y,
                     int32_t box_width,
                     int32_t box_height,
                     float size,
                     uint32_t color)
{
  if (!font || !text)
    return;

  /* Measure text width */
  int32_t text_width = measure_text_width(font, text, size);

  /* Calculate centered position */
  int32_t text_x = box_x + (box_width - text_width) / 2;

  /* Vertical centering (approximate - use small offset for better visual
   * balance) */
  float scale = stbtt_ScaleForPixelHeight(font, size);
  int ascent, descent, line_gap;
  stbtt_GetFontVMetrics(font, &ascent, &descent, &line_gap);
  int32_t text_height = (int32_t)((ascent - descent) * scale);
  int32_t text_y
    = box_y + (box_height - text_height) / 2 + (int32_t)(descent * scale);

  /* Render at centered position */
  render_text(framebuffer, stride, width, height, font, text, text_x, text_y,
              size, color);
}
