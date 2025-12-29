// SVG Resource Management
// Provides SVG icon loading and rasterization for OSD rendering
//
// This module handles SVG file loading using nanosvg, parsing vector
// graphics and preparing them for rasterization to bitmap.

#ifndef RESOURCES_SVG_H
#define RESOURCES_SVG_H

#include <stdbool.h>

// Forward declare NSVGimage to avoid including nanosvg.h in header
typedef struct NSVGimage NSVGimage;

// ════════════════════════════════════════════════════════════
// SVG RESOURCE STRUCTURE
// ════════════════════════════════════════════════════════════

// SVG resource handle
//
// Manages parsed SVG vector image data for icon rendering.
// Created by svg_load(), destroyed by svg_free().
typedef struct svg_resource_t
{
  NSVGimage *image; // nanosvg parsed image structure
  bool valid;       // True if SVG loaded and parsed successfully
} svg_resource_t;

// ════════════════════════════════════════════════════════════
// SVG LOADING AND MANAGEMENT
// ════════════════════════════════════════════════════════════

// Load an SVG file
//
// Parses SVG file from disk using nanosvg library.
// Loaded SVG can be rasterized to bitmap for rendering.
//
// Parameters:
//   svg:  SVG resource structure to initialize
//   path: Path to SVG file
//
// Returns:
//   true if SVG loaded and parsed successfully
//   false on error (file not found, invalid SVG format)
//
// Example:
//   svg_resource_t sun_icon = {0};
//   if (svg_load(&sun_icon, "resources/icons/sun.svg")) {
//     // Rasterize and render...
//     svg_free(&sun_icon);
//   }
//
// Notes:
//   - Caller must call svg_free() when done to release memory
//   - Uses logging system (LOG_DEBUG, LOG_INFO, LOG_ERROR)
//   - Sets svg.valid = true on success
//   - Parses with 96 DPI and "px" units
bool svg_load(svg_resource_t *svg, const char *path);

// Free SVG resource memory
//
// Releases parsed SVG image data.
// Safe to call on uninitialized or already-freed SVGs.
//
// Parameters:
//   svg: SVG resource to free
//
// Example:
//   svg_resource_t icon = {0};
//   svg_load(&icon, "icon.svg");
//   // ... use icon ...
//   svg_free(&icon);  // Clean up
void svg_free(svg_resource_t *svg);

// ════════════════════════════════════════════════════════════
// SVG VALIDATION AND QUERIES
// ════════════════════════════════════════════════════════════

// Check if SVG resource is valid and ready for use
//
// Parameters:
//   svg: SVG resource to check
//
// Returns:
//   true if SVG was loaded successfully and is ready for rendering
//   false if SVG is invalid, not loaded, or was freed
static inline bool
svg_is_valid(const svg_resource_t *svg)
{
  return svg && svg->valid && svg->image;
}

// Get SVG dimensions
//
// Parameters:
//   svg:    SVG resource
//   width:  Output width in pixels (can be NULL)
//   height: Output height in pixels (can be NULL)
//
// Returns:
//   true if SVG is valid and dimensions retrieved
//   false if SVG is invalid
//
// Example:
//   float w, h;
//   if (svg_get_dimensions(&icon, &w, &h)) {
//     printf("Icon size: %.0fx%.0f\n", w, h);
//   }
bool svg_get_dimensions(const svg_resource_t *svg, float *width, float *height);

// ════════════════════════════════════════════════════════════
// SVG RENDERING
// ════════════════════════════════════════════════════════════

#include "core/framebuffer.h"

// Render SVG to framebuffer
//
// Rasterizes the SVG at specified position and size, then blends
// it to the framebuffer.
//
// Parameters:
//   fb:     Target framebuffer
//   svg:    SVG resource to render
//   x, y:   Position to render at (top-left corner)
//   width:  Width in pixels to render
//   height: Height in pixels to render
//
// Example:
//   svg_render(&fb, &icon, 100, 100, 64, 64);
void svg_render(framebuffer_t *fb,
                const svg_resource_t *svg,
                int x,
                int y,
                int width,
                int height);

// Render SVG to framebuffer with alpha modifier
//
// Same as svg_render but applies an additional alpha multiplier
// for transparency effects (e.g., ghosted/behind indicators).
//
// Parameters:
//   fb:     Target framebuffer
//   svg:    SVG resource to render
//   x, y:   Position to render at (top-left corner)
//   width:  Width in pixels to render
//   height: Height in pixels to render
//   alpha:  Alpha multiplier (0.0 = fully transparent, 1.0 = fully opaque)
//
// Example:
//   svg_render_with_alpha(&fb, &icon, 100, 100, 64, 64, 0.5f);  // 50% opacity
void svg_render_with_alpha(framebuffer_t *fb,
                           const svg_resource_t *svg,
                           int x,
                           int y,
                           int width,
                           int height,
                           float alpha);

#endif // RESOURCES_SVG_H
