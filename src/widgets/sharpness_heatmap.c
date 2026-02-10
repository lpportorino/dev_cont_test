/**
 * @file sharpness_heatmap.c
 * @brief 8x8 sharpness heatmap widget
 *
 * Renders a color-coded 8x8 grid of sharpness scores from the CvMeta
 * opaque payload (pyramid level 3). Blue (0.0) -> green (0.5) -> red (1.0).
 */

#include "widgets/sharpness_heatmap.h"

#include "core/framebuffer.h"
#include "osd_state.h"
#include "rendering/blending.h"
#include "rendering/primitives.h"
#include "rendering/text.h"
#include "utils/logging.h"

#include <stdio.h>

#define HEATMAP_GRID_SIZE 8
#define HEATMAP_OUTLINE_THICKNESS 1

/**
 * Interpolate sharpness value to color.
 * 0.0 = blue, 0.5 = green, 1.0 = red, with semi-transparency.
 */
static uint32_t
sharpness_to_color(float value)
{
  if (value < 0.0f)
    value = 0.0f;
  if (value > 1.0f)
    value = 1.0f;

  uint8_t r, g, b;

  if (value < 0.5f)
    {
      // Blue -> Green
      float t = value * 2.0f;
      r       = 0;
      g       = (uint8_t)(t * 255.0f);
      b       = (uint8_t)((1.0f - t) * 255.0f);
    }
  else
    {
      // Green -> Red
      float t = (value - 0.5f) * 2.0f;
      r       = (uint8_t)(t * 255.0f);
      g       = (uint8_t)((1.0f - t) * 255.0f);
      b       = 0;
    }

  return color_make_argb(200, r, g, b);
}

bool
sharpness_heatmap_render(osd_context_t *ctx, const osd_state_t *state)
{
  (void)state;

  if (!ctx->config.sharpness_heatmap.enabled)
    {
      return false;
    }

  osd_sharpness_data_t data;
  if (!osd_state_get_sharpness(ctx, &data) || !data.valid)
    {
      return false;
    }

  framebuffer_t fb    = osd_ctx_get_framebuffer(ctx);
  int cell_size       = ctx->config.sharpness_heatmap.cell_size;
  int grid_px         = HEATMAP_GRID_SIZE * cell_size;
  int x0              = ctx->config.sharpness_heatmap.pos_x;
  int y0              = ctx->config.sharpness_heatmap.pos_y;
  int label_font_size = ctx->config.sharpness_heatmap.label_font_size;

  // Optional label above grid
  if (ctx->config.sharpness_heatmap.show_label)
    {
      char label[32];
      snprintf(label, sizeof(label), "Sharp: %.3f", data.global_score);
      text_render_with_outline(&fb, &ctx->font_variant_info, label, x0,
                               y0 - label_font_size - 2, 0xFFFFFFFF, 0xFF000000,
                               label_font_size, 1);
    }

  // White border around grid
  draw_rect_outline(&fb, x0 - 1, y0 - 1, grid_px + 2, grid_px + 2, 0xC0FFFFFF,
                    1.0f);

  // Render 8x8 grid cells
  int count = data.grid_count;
  if (count > 64)
    count = 64;

  for (int i = 0; i < count; i++)
    {
      int row    = i / HEATMAP_GRID_SIZE;
      int col    = i % HEATMAP_GRID_SIZE;
      int cx     = x0 + col * cell_size;
      int cy     = y0 + row * cell_size;
      uint32_t c = sharpness_to_color(data.grid_8x8[i]);
      draw_rect_filled(&fb, cx, cy, cell_size, cell_size, c);
    }

  return true;
}
