#include "rendering/text.h"

#include "rendering/blending.h"

// stb_truetype for font rendering
#ifndef isnan
#define isnan(x) __builtin_isnan(x)
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-function-declaration"

#include "stb_truetype.h"

#pragma clang diagnostic pop

// ════════════════════════════════════════════════════════════
// INTERNAL TEXT RENDERING
// ════════════════════════════════════════════════════════════

// Internal function to render text at specified position with offset
static void
text_render_internal(framebuffer_t *fb,
                     const font_resource_t *font,
                     const char *text,
                     int x,
                     int y,
                     uint32_t color,
                     int font_size,
                     int offset_x,
                     int offset_y)
{
  if (!font_is_valid(font) || !text || !text[0])
    return;

  // Calculate scale for desired pixel height
  float scale = stbtt_ScaleForPixelHeight(font->info, font_size);

  // Get font metrics
  int ascent, descent, line_gap;
  stbtt_GetFontVMetrics(font->info, &ascent, &descent, &line_gap);

  int baseline = (int)(ascent * scale);
  int pen_x    = x + offset_x;
  int pen_y    = y + baseline + offset_y;

  // Render each character
  for (const char *p = text; *p; p++)
    {
      int advance, lsb;
      stbtt_GetCodepointHMetrics(font->info, *p, &advance, &lsb);

      // Get glyph bitmap
      int glyph_width, glyph_height, xoff, yoff;
      unsigned char *bitmap = stbtt_GetCodepointBitmap(
        font->info, 0, scale, *p, &glyph_width, &glyph_height, &xoff, &yoff);

      if (bitmap)
        {
          // Render glyph
          int glyph_x = pen_x + (int)(lsb * scale) + xoff;
          int glyph_y = pen_y + yoff;

          for (int gy = 0; gy < glyph_height; gy++)
            {
              for (int gx = 0; gx < glyph_width; gx++)
                {
                  int px = glyph_x + gx;
                  int py = glyph_y + gy;

                  if (framebuffer_in_bounds(fb, px, py))
                    {
                      unsigned char glyph_alpha = bitmap[gy * glyph_width + gx];
                      if (glyph_alpha > 0)
                        {
                          // Extract ARGB from color (internal format:
                          // 0xAABBGGRR)
                          uint32_t r            = color & 0xFF;
                          uint32_t g            = (color >> 8) & 0xFF;
                          uint32_t b            = (color >> 16) & 0xFF;
                          uint32_t config_alpha = (color >> 24) & 0xFF;

                          // Combine glyph alpha with config alpha
                          // glyph_alpha = anti-aliasing from font (0-255)
                          // config_alpha = user transparency setting (0-255)
                          uint32_t final_alpha
                            = (glyph_alpha * config_alpha) / 255;

                          // Create RGBA color with combined alpha
                          uint32_t pixel_color
                            = (final_alpha << 24) | (b << 16) | (g << 8) | r;

                          // Blend with background
                          framebuffer_blend_pixel(fb, px, py, pixel_color);
                        }
                    }
                }
            }

          stbtt_FreeBitmap(bitmap, NULL);
        }

      // Advance pen
      pen_x += (int)(advance * scale);

      // Kerning (if next char exists)
      if (p[1])
        {
          int kern = stbtt_GetCodepointKernAdvance(font->info, *p, p[1]);
          pen_x += (int)(kern * scale);
        }
    }
}

// ════════════════════════════════════════════════════════════
// PUBLIC TEXT RENDERING API
// ════════════════════════════════════════════════════════════

void
text_render_with_outline(framebuffer_t *fb,
                         const font_resource_t *font,
                         const char *text,
                         int x,
                         int y,
                         uint32_t color,
                         uint32_t outline_color,
                         int font_size,
                         int outline_thickness)
{
  if (!font_is_valid(font) || !text || !text[0])
    return;

  // Apply main color's alpha to outline color
  // This ensures outline transparency matches text transparency
  // Without this, semi-transparent text would have opaque black outline
  uint32_t main_alpha       = (color >> 24) & 0xFF;
  uint32_t outline_rgb      = outline_color & 0x00FFFFFF;
  uint32_t adjusted_outline = (main_alpha << 24) | outline_rgb;

  // Render outline/stroke first (if enabled)
  if (outline_thickness > 0)
    {
      // Render text multiple times with offsets to create outline effect
      // Use 8-direction offsets for circular outline
      for (int ox = -outline_thickness; ox <= outline_thickness; ox++)
        {
          for (int oy = -outline_thickness; oy <= outline_thickness; oy++)
            {
              if (ox == 0 && oy == 0)
                continue; // Skip center (main text)
              text_render_internal(fb, font, text, x, y, adjusted_outline,
                                   font_size, ox, oy);
            }
        }
    }

  // Render main text on top
  text_render_internal(fb, font, text, x, y, color, font_size, 0, 0);
}

void
text_render(framebuffer_t *fb,
            const font_resource_t *font,
            const char *text,
            int x,
            int y,
            uint32_t color,
            int font_size)
{
  text_render_with_outline(fb, font, text, x, y, color, 0xFF000000, font_size,
                           0);
}

// ════════════════════════════════════════════════════════════
// TEXT MEASUREMENT
// ════════════════════════════════════════════════════════════

int
text_measure_width(const font_resource_t *font, const char *text, int font_size)
{
  if (!font_is_valid(font) || !text || !text[0])
    return 0;

  // Calculate scale for desired pixel height
  float scale = stbtt_ScaleForPixelHeight(font->info, font_size);

  int total_width = 0;

  // Measure each character
  for (const char *p = text; *p; p++)
    {
      int advance, lsb;
      stbtt_GetCodepointHMetrics(font->info, *p, &advance, &lsb);

      // Add character advance
      total_width += (int)(advance * scale);

      // Add kerning (if next char exists)
      if (p[1])
        {
          int kern = stbtt_GetCodepointKernAdvance(font->info, *p, p[1]);
          total_width += (int)(kern * scale);
        }
    }

  return total_width;
}
