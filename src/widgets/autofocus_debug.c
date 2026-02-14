/**
 * @file autofocus_debug.c
 * @brief Autofocus debug panel widget
 *
 * Renders:
 * - Focus and zoom lens position sliders
 * - 8x8 sharpness heatmap
 * - 10-second sharpness history chart (simple time plot)
 */

#include "widgets/autofocus_debug.h"

#include "core/context_helpers.h"
#include "core/framebuffer.h"
#include "osd_state.h"
#include "rendering/blending.h"
#include "rendering/primitives.h"
#include "rendering/text.h"

#include <stdio.h>
#include <string.h>

// ════════════════════════════════════════════════════════════
// LAYOUT CONSTANTS
// ════════════════════════════════════════════════════════════

#define BOX_SIZE 96
#define CHART_WIDTH 288
#define SLIDER_WIDTH 20
#define TRACK_WIDTH 4
#define KNOB_WIDTH 16
#define KNOB_HEIGHT 6
#define ELEMENT_GAP 8
#define LABEL_HEIGHT 14

#define WIDGET_TOTAL_WIDTH 448

#define HEATMAP_GRID_SIZE 8
#define HEATMAP_CELL_SIZE 12

#define HISTORY_SIZE 300
#define HISTORY_WINDOW_US (10ULL * 1000000ULL)

// ════════════════════════════════════════════════════════════
// COLORS (0xAABBGGRR format)
// ════════════════════════════════════════════════════════════

#define COLOR_BORDER 0xC0FFFFFF
#define COLOR_TRACK 0x80FFFFFF
#define COLOR_FOCUS_KNOB 0xFFFFD400
#define COLOR_ZOOM_KNOB 0xFFD400FF
#define COLOR_HISTORY_BAR 0xC0D400FF
#define COLOR_TEXT 0xFFFFFFFF
#define COLOR_TEXT_OUTLINE 0xFF000000

// ════════════════════════════════════════════════════════════
// STATIC STATE - simple ring buffer for sharpness history
// ════════════════════════════════════════════════════════════

static struct
{
  float sharpness;
  uint64_t time_us;
} s_history[HISTORY_SIZE];

static int s_history_count = 0;
static int s_history_idx   = 0;

// ════════════════════════════════════════════════════════════
// HELPER: Sharpness to color (blue->green->red)
// ════════════════════════════════════════════════════════════

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
      float t = value * 2.0f;
      r       = 0;
      g       = (uint8_t)(t * 255.0f);
      b       = (uint8_t)((1.0f - t) * 255.0f);
    }
  else
    {
      float t = (value - 0.5f) * 2.0f;
      r       = (uint8_t)(t * 255.0f);
      g       = (uint8_t)((1.0f - t) * 255.0f);
      b       = 0;
    }
  return color_make_argb(200, r, g, b);
}

// ════════════════════════════════════════════════════════════
// RENDER: Slider with knob
// ════════════════════════════════════════════════════════════

static void
render_slider(framebuffer_t *fb,
              const font_resource_t *font,
              int x,
              int y,
              double position,
              uint32_t knob_color,
              const char *label)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%s.%02d", label, (int)(position * 100) % 100);
  text_render_with_outline(fb, font, buf, x, y, COLOR_TEXT, COLOR_TEXT_OUTLINE,
                           10, 1);

  int box_y = y + LABEL_HEIGHT;
  draw_rect_outline(fb, x, box_y, SLIDER_WIDTH, BOX_SIZE, COLOR_BORDER, 1.0f);

  int track_x = x + (SLIDER_WIDTH - TRACK_WIDTH) / 2;
  draw_rect_filled(fb, track_x, box_y + 4, TRACK_WIDTH, BOX_SIZE - 8,
                   COLOR_TRACK);

  int knob_travel = BOX_SIZE - 8 - KNOB_HEIGHT;
  int knob_y      = box_y + 4 + (int)((1.0 - position) * knob_travel);
  int knob_x      = x + (SLIDER_WIDTH - KNOB_WIDTH) / 2;
  draw_rect_filled(fb, knob_x, knob_y, KNOB_WIDTH, KNOB_HEIGHT, knob_color);
  draw_rect_outline(fb, knob_x, knob_y, KNOB_WIDTH, KNOB_HEIGHT, COLOR_BORDER,
                    1.0f);
}

// ════════════════════════════════════════════════════════════
// RENDER: 8x8 Sharpness heatmap
// ════════════════════════════════════════════════════════════

static void
render_heatmap(framebuffer_t *fb,
               const font_resource_t *font,
               int x,
               int y,
               const osd_sharpness_data_t *data)
{
  char label[24];
  snprintf(label, sizeof(label), "Sharp: %.3f", data->global_score);
  text_render_with_outline(fb, font, label, x, y, COLOR_TEXT,
                           COLOR_TEXT_OUTLINE, 10, 1);

  int grid_y = y + LABEL_HEIGHT;
  draw_rect_outline(fb, x - 1, grid_y - 1, BOX_SIZE + 2, BOX_SIZE + 2,
                    COLOR_BORDER, 1.0f);

  int count = data->grid_count;
  if (count > 64)
    count = 64;

  float min_val = data->grid_8x8[0];
  float max_val = data->grid_8x8[0];
  for (int i = 1; i < count; i++)
    {
      if (data->grid_8x8[i] < min_val)
        min_val = data->grid_8x8[i];
      if (data->grid_8x8[i] > max_val)
        max_val = data->grid_8x8[i];
    }
  float range = max_val - min_val;

  for (int i = 0; i < count; i++)
    {
      int row = i / HEATMAP_GRID_SIZE;
      int col = i % HEATMAP_GRID_SIZE;
      int cx  = x + col * HEATMAP_CELL_SIZE;
      int cy  = grid_y + row * HEATMAP_CELL_SIZE;
      float norm
        = (range > 0.001f) ? (data->grid_8x8[i] - min_val) / range : 0.5f;
      draw_rect_filled(fb, cx, cy, HEATMAP_CELL_SIZE, HEATMAP_CELL_SIZE,
                       sharpness_to_color(norm));
    }
}

// ════════════════════════════════════════════════════════════
// RENDER: Simple 10-second history chart
// ════════════════════════════════════════════════════════════

static void
render_history_chart(framebuffer_t *fb, int x, int y, uint64_t now_us)
{
  int chart_y = y + LABEL_HEIGHT;
  draw_rect_outline(fb, x, chart_y, CHART_WIDTH, BOX_SIZE, COLOR_BORDER, 1.0f);

  if (s_history_count == 0)
    return;

  uint64_t cutoff_us
    = (now_us > HISTORY_WINDOW_US) ? (now_us - HISTORY_WINDOW_US) : 0;

  // Collect visible samples and find min/max
  float min_s       = 1.0f;
  float max_s       = 0.0f;
  int visible_count = 0;
  int visible_indices[HISTORY_SIZE];

  for (int i = 0; i < s_history_count; i++)
    {
      if (s_history[i].time_us >= cutoff_us)
        {
          float s = s_history[i].sharpness;
          if (s < min_s)
            min_s = s;
          if (s > max_s)
            max_s = s;
          visible_indices[visible_count++] = i;
        }
    }

  if (visible_count == 0)
    return;

  float range = max_s - min_s;
  if (range < 0.001f)
    range = 1.0f;

  // Draw bars
  float bar_w = (float)(CHART_WIDTH - 4) / visible_count;
  for (int i = 0; i < visible_count; i++)
    {
      int idx    = visible_indices[i];
      float norm = (s_history[idx].sharpness - min_s) / range;

      int bx = x + 2 + (int)(i * bar_w);
      int bh = (int)(norm * (BOX_SIZE - 4));
      if (bh < 1)
        bh = 1;
      int by = chart_y + BOX_SIZE - 2 - bh;
      draw_rect_filled(fb, bx, by, (int)bar_w + 1, bh, COLOR_HISTORY_BAR);
    }
}

// ════════════════════════════════════════════════════════════
// MAIN RENDER FUNCTION
// ════════════════════════════════════════════════════════════

bool
autofocus_debug_render(osd_context_t *ctx, osd_state_t *pb_state)
{
  if (!ctx->config.autofocus_debug.enabled)
    return false;

  // Get camera day data
  osd_camera_day_data_t cam = { 0 };
  bool has_cam              = false;
  if (pb_state)
    has_cam = osd_state_get_camera_day(pb_state, &cam);

  // Get sharpness data
  osd_sharpness_data_t sharp = { 0 };
  bool has_sharp             = osd_state_get_sharpness(ctx, &sharp);
  if (!has_sharp || !sharp.valid)
    return false;

  // Get monotonic time
  uint64_t now_us = 0;
  if (pb_state)
    now_us = osd_state_get_monotonic_time_us(pb_state);

  // Add to history (simple, no heuristics)
  s_history[s_history_idx].sharpness = sharp.global_score;
  s_history[s_history_idx].time_us   = now_us;
  s_history_idx                      = (s_history_idx + 1) % HISTORY_SIZE;
  if (s_history_count < HISTORY_SIZE)
    s_history_count++;

  // Layout
  double focus_pos = has_cam ? cam.focus_pos : 0.0;
  double zoom_pos  = has_cam ? cam.zoom_pos : 0.0;
  int pos_y        = ctx->config.autofocus_debug.pos_y;

  framebuffer_t fb = osd_ctx_get_framebuffer(ctx);
  int current_x    = (fb.width - WIDGET_TOTAL_WIDTH) / 2;

  // 1. Focus slider
  render_slider(&fb, &ctx->font_variant_info, current_x, pos_y, focus_pos,
                COLOR_FOCUS_KNOB, "F");
  current_x += SLIDER_WIDTH + ELEMENT_GAP;

  // 2. Zoom slider
  render_slider(&fb, &ctx->font_variant_info, current_x, pos_y, zoom_pos,
                COLOR_ZOOM_KNOB, "Z");
  current_x += SLIDER_WIDTH + ELEMENT_GAP;

  // 3. Heatmap
  render_heatmap(&fb, &ctx->font_variant_info, current_x, pos_y, &sharp);
  current_x += BOX_SIZE + ELEMENT_GAP;

  // 4. History chart (simple 10s plot)
  render_history_chart(&fb, current_x, pos_y, now_us);

  return true;
}
