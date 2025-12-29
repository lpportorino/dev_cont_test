/**
 * @file utils.h
 * @brief OSD utility functions - rendering, drawing, and text utilities
 *
 * Reusable utility functions for OSD rendering including:
 * - Color conversion
 * - Rectangle operations
 * - Primitive drawing (rectangles, circles)
 * - Text rendering with TrueType fonts (left-aligned and centered)
 */

#ifndef UTILS_H
#define UTILS_H

#include "stb_truetype.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /* ==================== OSD Context (forward declaration) ====================
   */

  /**
   * Minimal context structure for utility functions
   * Full definition in wasm_osd_day_lib.c
   */
  typedef struct wasm_osd_context wasm_osd_context_t;

  /* ==================== Color Utilities ==================== */

  /**
   * Convert Color struct to 32-bit RGB value for text rendering
   * @param c Color struct with RGBA components
   * @return 32-bit value in 0xRRGGBB format (alpha is discarded)
   */
  uint32_t color_to_u32(Color c);

  /* ==================== Rectangle Utilities ==================== */

  /**
   * Clear a rectangular region in the framebuffer (set all pixels to
   * transparent black)
   * @param fb Framebuffer pointer
   * @param stride Row stride in bytes (width * 4 for RGBA)
   * @param x Left edge of rectangle
   * @param y Top edge of rectangle
   * @param w Width of rectangle
   * @param h Height of rectangle
   */
  void clear_rect(
    uint8_t *fb, int32_t stride, int32_t x, int32_t y, int32_t w, int32_t h);

  /* ==================== Primitive Drawing ==================== */

  /**
   * Draw a filled rectangle with the specified color
   * @param framebuffer Framebuffer pointer
   * @param stride Row stride in bytes
   * @param width Framebuffer width
   * @param height Framebuffer height
   * @param x Left edge
   * @param y Top edge
   * @param w Width
   * @param h Height
   * @param c Color (RGBA)
   */
  void draw_filled_rect(uint8_t *framebuffer,
                        int32_t stride,
                        int32_t width,
                        int32_t height,
                        int32_t x,
                        int32_t y,
                        int32_t w,
                        int32_t h,
                        Color c);

  /**
   * Draw a filled circle using Bresenham-style algorithm
   * @param framebuffer Framebuffer pointer
   * @param stride Row stride in bytes
   * @param width Framebuffer width
   * @param height Framebuffer height
   * @param cx Center X coordinate
   * @param cy Center Y coordinate
   * @param radius Circle radius in pixels
   * @param c Color (RGBA)
   */
  void draw_filled_circle(uint8_t *framebuffer,
                          int32_t stride,
                          int32_t width,
                          int32_t height,
                          int32_t cx,
                          int32_t cy,
                          int32_t radius,
                          Color c);

  /**
   * Draw a circle outline (ring) with specified thickness
   * @param framebuffer Framebuffer pointer
   * @param stride Row stride in bytes
   * @param width Framebuffer width
   * @param height Framebuffer height
   * @param cx Center X coordinate
   * @param cy Center Y coordinate
   * @param radius Outer radius in pixels
   * @param thickness Ring thickness in pixels
   * @param c Color (RGBA)
   */
  void draw_circle_outline(uint8_t *framebuffer,
                           int32_t stride,
                           int32_t width,
                           int32_t height,
                           int32_t cx,
                           int32_t cy,
                           int32_t radius,
                           int32_t thickness,
                           Color c);

  /* ==================== Text Rendering ==================== */

  /**
   * Measure text width using TrueType font metrics
   * @param font TrueType font info
   * @param text Null-terminated string to measure
   * @param size Font size in pixels
   * @return Text width in pixels
   */
  int32_t
  measure_text_width(stbtt_fontinfo *font, const char *text, float size);

  /**
   * Render TrueType text with alpha blending (left-aligned)
   *
   * Uses stb_truetype to rasterize glyphs and blends them onto the framebuffer
   * using standard alpha composition: dst = (src * alpha + dst * (1-alpha)) /
   * 255
   *
   * @param framebuffer Framebuffer pointer
   * @param stride Row stride in bytes
   * @param width Framebuffer width
   * @param height Framebuffer height
   * @param font TrueType font info
   * @param text Null-terminated string to render
   * @param x Left edge of text baseline
   * @param y Top edge of text baseline
   * @param size Font size in pixels
   * @param color RGB color in 0xRRGGBB format
   */
  void render_text(uint8_t *framebuffer,
                   int32_t stride,
                   int32_t width,
                   int32_t height,
                   stbtt_fontinfo *font,
                   const char *text,
                   int32_t x,
                   int32_t y,
                   float size,
                   uint32_t color);

  /**
   * Render TrueType text centered within a bounding box
   *
   * Measures the text width and centers it horizontally and vertically
   * within the specified box dimensions.
   *
   * @param framebuffer Framebuffer pointer
   * @param stride Row stride in bytes
   * @param width Framebuffer width
   * @param height Framebuffer height
   * @param font TrueType font info
   * @param text Null-terminated string to render
   * @param box_x Left edge of bounding box
   * @param box_y Top edge of bounding box
   * @param box_width Width of bounding box
   * @param box_height Height of bounding box
   * @param size Font size in pixels
   * @param color RGB color in 0xRRGGBB format
   */
  void render_text_centered(uint8_t *framebuffer,
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
                            uint32_t color);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H */
