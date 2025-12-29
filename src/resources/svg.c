#include "resources/svg.h"

#include "utils/logging.h"

#include <stdlib.h>
#include <string.h>

// nanosvg implementation (single-header library)
// Note: nanosvg uses math functions without including math.h
// WASI SDK strict mode would error, so we disable that warning
#ifndef isnan
#define isnan(x) __builtin_isnan(x)
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-function-declaration"

// Suppress false positive warnings from vendor library (nanosvg)
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h" // NOLINT

#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h" // NOLINT

#pragma clang diagnostic pop

// ════════════════════════════════════════════════════════════
// SVG LOADING IMPLEMENTATION
// ════════════════════════════════════════════════════════════

bool
svg_load(svg_resource_t *svg, const char *path)
{
  if (!svg || !path)
    {
      LOG_ERROR("Invalid arguments to svg_load()");
      return false;
    }

  // Initialize SVG structure
  memset(svg, 0, sizeof(svg_resource_t));

  LOG_DEBUG("Loading SVG from: %s", path);

  // Parse SVG file
  // Units: "px" (pixels)
  // DPI: 96.0 (standard web DPI)
  NSVGimage *image = nsvgParseFromFile(path, "px", 96.0f);
  if (!image)
    {
      LOG_ERROR("Failed to parse SVG file: %s", path);
      return false;
    }

  LOG_INFO("SVG loaded: %.0fx%.0f", image->width, image->height);

  svg->image = image;
  svg->valid = true;
  return true;
}

// ════════════════════════════════════════════════════════════
// SVG CLEANUP IMPLEMENTATION
// ════════════════════════════════════════════════════════════

void
svg_free(svg_resource_t *svg)
{
  if (!svg)
    {
      return;
    }

  if (svg->image)
    {
      nsvgDelete(svg->image);
      svg->image = NULL;
    }

  svg->valid = false;
}

// ════════════════════════════════════════════════════════════
// SVG QUERY IMPLEMENTATION
// ════════════════════════════════════════════════════════════

bool
svg_get_dimensions(const svg_resource_t *svg, float *width, float *height)
{
  if (!svg_is_valid(svg))
    {
      return false;
    }

  if (width)
    {
      *width = svg->image->width;
    }

  if (height)
    {
      *height = svg->image->height;
    }

  return true;
}

// ════════════════════════════════════════════════════════════
// SVG RENDERING IMPLEMENTATION
// ════════════════════════════════════════════════════════════

#include "core/framebuffer.h"
#include "rendering/blending.h"

void
svg_render(framebuffer_t *fb,
           const svg_resource_t *svg,
           int x,
           int y,
           int width,
           int height)
{
  if (!fb || !svg_is_valid(svg) || width <= 0 || height <= 0)
    {
      return;
    }

  // Create rasterizer
  NSVGrasterizer *rast = nsvgCreateRasterizer();
  if (!rast)
    {
      LOG_ERROR("Failed to create SVG rasterizer");
      return;
    }

  // Allocate buffer for rasterized image (RGBA)
  // Cast to size_t before multiplication to avoid overflow
  size_t img_size    = (size_t)width * (size_t)height * 4;
  unsigned char *img = (unsigned char *)malloc(img_size);
  if (!img)
    {
      LOG_ERROR("Failed to allocate SVG rasterization buffer");
      nsvgDeleteRasterizer(rast);
      return;
    }

  // Zero-initialize buffer (defensive programming)
  memset(img, 0, img_size);

  // Calculate scale factor
  float scale_x = (float)width / svg->image->width;
  float scale_y = (float)height / svg->image->height;
  float scale   = (scale_x < scale_y) ? scale_x : scale_y;

  // Rasterize SVG to buffer (RGBA format)
  nsvgRasterize(rast, svg->image, 0, 0, scale, img, width, height, width * 4);

  // Blend rasterized image to framebuffer
  for (int py = 0; py < height; py++)
    {
      for (int px = 0; px < width; px++)
        {
          int img_idx = (py * width + px) * 4;

          // Extract RGBA components (nsvgRasterize outputs RGBA)
          uint8_t r = img[img_idx + 0];
          uint8_t g = img[img_idx + 1];
          uint8_t b = img[img_idx + 2];
          uint8_t a = img[img_idx + 3];

          // Assemble RGBA color (0xAABBGGRR format)
          uint32_t color = (a << 24) | (b << 16) | (g << 8) | r;

          // Blend to framebuffer
          int screen_x = x + px;
          int screen_y = y + py;
          if (screen_x >= 0 && screen_x < (int)fb->width && screen_y >= 0
              && screen_y < (int)fb->height)
            {
              framebuffer_blend_pixel(fb, screen_x, screen_y, color);
            }
        }
    }

  // Cleanup
  free(img);
  nsvgDeleteRasterizer(rast);
}

void
svg_render_with_alpha(framebuffer_t *fb,
                      const svg_resource_t *svg,
                      int x,
                      int y,
                      int width,
                      int height,
                      float alpha)
{
  if (!fb || !svg_is_valid(svg) || width <= 0 || height <= 0)
    {
      return;
    }

  // Clamp alpha to valid range
  if (alpha <= 0.0f)
    return; // Fully transparent, nothing to render
  if (alpha > 1.0f)
    alpha = 1.0f;

  // Create rasterizer
  NSVGrasterizer *rast = nsvgCreateRasterizer();
  if (!rast)
    {
      LOG_ERROR("Failed to create SVG rasterizer");
      return;
    }

  // Allocate buffer for rasterized image (RGBA)
  size_t img_size    = (size_t)width * (size_t)height * 4;
  unsigned char *img = (unsigned char *)malloc(img_size);
  if (!img)
    {
      LOG_ERROR("Failed to allocate SVG rasterization buffer");
      nsvgDeleteRasterizer(rast);
      return;
    }

  memset(img, 0, img_size);

  // Calculate scale factor
  float scale_x = (float)width / svg->image->width;
  float scale_y = (float)height / svg->image->height;
  float scale   = (scale_x < scale_y) ? scale_x : scale_y;

  // Rasterize SVG to buffer (RGBA format)
  nsvgRasterize(rast, svg->image, 0, 0, scale, img, width, height, width * 4);

  // Blend rasterized image to framebuffer with alpha modifier
  for (int py = 0; py < height; py++)
    {
      for (int px = 0; px < width; px++)
        {
          int img_idx = (py * width + px) * 4;

          // Extract RGBA components
          uint8_t r = img[img_idx + 0];
          uint8_t g = img[img_idx + 1];
          uint8_t b = img[img_idx + 2];
          uint8_t a = img[img_idx + 3];

          // Apply alpha modifier
          a = (uint8_t)(a * alpha);

          // Assemble RGBA color (0xAABBGGRR format)
          uint32_t color = (a << 24) | (b << 16) | (g << 8) | r;

          // Blend to framebuffer
          int screen_x = x + px;
          int screen_y = y + py;
          if (screen_x >= 0 && screen_x < (int)fb->width && screen_y >= 0
              && screen_y < (int)fb->height)
            {
              framebuffer_blend_pixel(fb, screen_x, screen_y, color);
            }
        }
    }

  // Cleanup
  free(img);
  nsvgDeleteRasterizer(rast);
}
