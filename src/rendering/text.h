// Text Rendering Module
// Provides TrueType font rendering using stb_truetype
//
// This module handles text rasterization and rendering with support for:
// - Anti-aliased glyph rendering
// - Kerning and proper text layout
// - Outline/stroke effects for visibility
// - Alpha blending with background

#ifndef RENDERING_TEXT_H
#define RENDERING_TEXT_H

#include "core/framebuffer.h"
#include "resources/font.h"

#include <stdint.h>

// ════════════════════════════════════════════════════════════
// TEXT RENDERING
// ════════════════════════════════════════════════════════════

// Render text with outline/stroke effect
//
// Renders anti-aliased text with optional outline for better visibility
// on varying backgrounds. Uses stb_truetype for glyph rasterization.
//
// Parameters:
//   fb:                Framebuffer to render into
//   font:              Font resource (loaded TrueType font)
//   text:              Text string to render (UTF-8)
//   x, y:              Top-left position for text baseline
//   color:             Text color (RGBA format: 0xAABBGGRR)
//   outline_color:     Outline color (RGBA format: 0xAABBGGRR)
//   font_size:         Font size in pixels (height)
//   outline_thickness: Outline thickness in pixels (0 = no outline)
//
// Example:
//   // White text with black outline
//   text_render_with_outline(&fb, &font, "Hello", 100, 50,
//                            0xFFFFFFFF, 0xFF000000, 24, 2);
//
// Notes:
//   - Outline rendered first, then main text on top
//   - Outline uses 8-direction offsets for circular appearance
//   - Glyph alpha blended with background
//   - Kerning applied between characters
//   - Returns silently if font invalid or text empty
void text_render_with_outline(framebuffer_t *fb,
                              const font_resource_t *font,
                              const char *text,
                              int x,
                              int y,
                              uint32_t color,
                              uint32_t outline_color,
                              int font_size,
                              int outline_thickness);

// Render text without outline
//
// Simple text rendering without stroke effect.
// Convenience wrapper for text_render_with_outline() with no outline.
//
// Parameters:
//   fb:        Framebuffer to render into
//   font:      Font resource (loaded TrueType font)
//   text:      Text string to render (UTF-8)
//   x, y:      Top-left position for text baseline
//   color:     Text color (RGBA format: 0xAABBGGRR)
//   font_size: Font size in pixels (height)
//
// Example:
//   // Simple white text
//   text_render(&fb, &font, "Hello World", 100, 50, 0xFFFFFFFF, 24);
void text_render(framebuffer_t *fb,
                 const font_resource_t *font,
                 const char *text,
                 int x,
                 int y,
                 uint32_t color,
                 int font_size);

// ════════════════════════════════════════════════════════════
// TEXT MEASUREMENT
// ════════════════════════════════════════════════════════════

// Measure text width in pixels
//
// Calculates the horizontal width of a text string when rendered
// at a specific font size. Includes kerning between characters.
//
// Parameters:
//   font:      Font resource (loaded TrueType font)
//   text:      Text string to measure (UTF-8)
//   font_size: Font size in pixels (height)
//
// Returns:
//   Width of text in pixels (including kerning)
//   Returns 0 if font invalid or text empty
//
// Example:
//   int width = text_measure_width(&font, "45.2°/s", 14);
//   int center_x = screen_center_x - (width / 2);  // Center align
int text_measure_width(const font_resource_t *font,
                       const char *text,
                       int font_size);

#endif // RENDERING_TEXT_H
