/**
 * @file roi.c
 * @brief ROI (Region of Interest) overlay widget
 *
 * Renders colored bounding boxes with labels for active ROI regions.
 * ROI data comes from JonGuiDataCV proto state fields (not opaque payloads).
 * Each ROI type (focus, track, zoom, fx) has a distinct color.
 */

#include "widgets/roi.h"

#include "core/framebuffer.h"
#include "osd_state.h"
#include "rendering/primitives.h"
#include "rendering/text.h"

#include <stdio.h>

/**
 * Render a single ROI rectangle with label
 */
static bool
render_single_roi(framebuffer_t *fb,
                  osd_context_t *ctx,
                  const osd_roi_t *roi,
                  uint32_t color,
                  const char *label,
                  float thickness,
                  int font_size)
{
  if (!roi->present)
    return false;

  float w = (float)ctx->width;
  float h = (float)ctx->height;

  // NDC [-1.0, 1.0] -> pixel coordinates
  // Note: ROI uses image-space NDC where y1=top (smaller Y = top)
  int px1 = (int)((roi->x1 + 1.0) / 2.0 * w);
  int py1 = (int)((roi->y1 + 1.0) / 2.0 * h);
  int px2 = (int)((roi->x2 + 1.0) / 2.0 * w);
  int py2 = (int)((roi->y2 + 1.0) / 2.0 * h);

  int bw = px2 - px1;
  int bh = py2 - py1;
  if (bw <= 0 || bh <= 0)
    return false;

  // Draw rectangle outline
  draw_rect_outline(fb, px1, py1, bw, bh, color, thickness);

  // Draw label above box
  int label_w = text_measure_width(&ctx->font_variant_info, label, font_size);
  int label_h = font_size + 2;
  int lx      = px1;
  int ly      = py1 - label_h - 1;
  if (ly < 0)
    ly = py1 + 1;

  draw_rect_filled(fb, lx, ly, label_w + 4, label_h, 0xA0000000);
  text_render_with_outline(fb, &ctx->font_variant_info, label, lx + 2, ly + 1,
                           color, 0xFF000000, font_size, 1);

  return true;
}

bool
roi_render(osd_context_t *ctx, const osd_state_t *state)
{
  if (!ctx->config.roi.enabled)
    return false;

#ifdef OSD_STREAM_THERMAL
  bool is_thermal = true;
#else
  bool is_thermal = false;
#endif

  osd_roi_data_t data;
  if (!osd_state_get_rois(state, is_thermal, &data) || !data.valid)
    return false;

  framebuffer_t fb      = osd_ctx_get_framebuffer(ctx);
  const roi_config_t *c = &ctx->config.roi;
  bool rendered         = false;

  rendered |= render_single_roi(&fb, ctx, &data.focus, c->color_focus, "FOCUS",
                                c->box_thickness, c->label_font_size);
  rendered |= render_single_roi(&fb, ctx, &data.track, c->color_track, "TRACK",
                                c->box_thickness, c->label_font_size);
  rendered |= render_single_roi(&fb, ctx, &data.zoom, c->color_zoom, "ZOOM",
                                c->box_thickness, c->label_font_size);
  rendered |= render_single_roi(&fb, ctx, &data.fx, c->color_fx, "FX",
                                c->box_thickness, c->label_font_size);

  return rendered;
}
