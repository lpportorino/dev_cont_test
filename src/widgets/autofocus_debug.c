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
#include "utils/math_decl.h"

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

#define HISTORY_SIZE 900
#define HISTORY_WINDOW_US (30ULL * 1000000ULL)

// ════════════════════════════════════════════════════════════
// COLORS (0xAABBGGRR format)
// ════════════════════════════════════════════════════════════

#define COLOR_BORDER 0xC0FFFFFF
#define COLOR_TRACK 0x80FFFFFF
#define COLOR_FOCUS_KNOB 0xFFFFD400
#define COLOR_ZOOM_KNOB 0xFFD400FF
#define COLOR_HISTORY_BAR 0xC0D400FF
#define COLOR_CURVE 0xFFFFFF00
#define COLOR_CURVE_FILL 0x40FFFF00
#define COLOR_TEXT 0xFFFFFFFF
#define COLOR_TEXT_OUTLINE 0xFF000000

#define SPLINE_SEGMENTS_PER_SPAN 8
#define SPLINE_THICKNESS 2.0f
#define EMA_ALPHA 0.1f

// ════════════════════════════════════════════════════════════
// STATIC STATE - robust ring buffer for sharpness history
// ════════════════════════════════════════════════════════════

typedef struct
{
  float sharpness;  // Raw value [0.0-1.0]
  float smoothed;   // EMA smoothed value (for curve)
  uint64_t time_us; // Monotonic timestamp
  uint8_t valid;    // Validation flag
} history_sample_t;

static history_sample_t s_history[HISTORY_SIZE];
static int s_history_count     = 0;
static int s_history_head      = 0;    // Oldest valid entry (read pointer)
static int s_history_tail      = 0;    // Next write position (write pointer)
static uint64_t s_last_time_us = 0;    // For monotonicity check
static float s_ema_value       = 0.0f; // Running EMA for curve

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
// HELPER: Add sample to history with validation and EMA
// ════════════════════════════════════════════════════════════

static void
history_add_sample(float sharpness, uint64_t time_us)
{
  // Validate sharpness - clamp to valid range
  // Check for NaN (NaN != NaN) and out-of-range values
  if (sharpness != sharpness || sharpness < 0.0f || sharpness > 1.0f)
    {
      // NaN becomes 0, out-of-range gets clamped
      if (sharpness != sharpness)
        sharpness = 0.0f;
      else
        sharpness = fmaxf(0.0f, fminf(1.0f, sharpness));
    }

  // Validate time monotonicity (allow small jitter up to 100ms)
  if (time_us < s_last_time_us && (s_last_time_us - time_us) > 100000ULL)
    {
      // Time went backwards by >100ms - likely reset, clear history
      s_history_count = 0;
      s_history_head  = 0;
      s_history_tail  = 0;
      s_ema_value     = sharpness;
    }
  s_last_time_us = time_us;

  // Initialize EMA on first sample
  if (s_history_count == 0)
    {
      s_ema_value = sharpness;
    }
  else
    {
      // EMA smoothing (alpha ~= 0.1 for ~10 sample smoothing)
      s_ema_value = EMA_ALPHA * sharpness + (1.0f - EMA_ALPHA) * s_ema_value;
    }

  // Write sample
  s_history[s_history_tail].sharpness = sharpness;
  s_history[s_history_tail].smoothed  = s_ema_value;
  s_history[s_history_tail].time_us   = time_us;
  s_history[s_history_tail].valid     = 1;

  // Advance tail (circular)
  s_history_tail = (s_history_tail + 1) % HISTORY_SIZE;

  // If buffer full, advance head (discard oldest)
  if (s_history_count == HISTORY_SIZE)
    {
      s_history_head = (s_history_head + 1) % HISTORY_SIZE;
    }
  else
    {
      s_history_count++;
    }
}

// ════════════════════════════════════════════════════════════
// HELPER: Catmull-Rom spline interpolation
// ════════════════════════════════════════════════════════════

static inline float
catmull_rom(float p0, float p1, float p2, float p3, float t)
{
  // Centripetal Catmull-Rom for smoothest curves
  float t2 = t * t;
  float t3 = t2 * t;
  return 0.5f
         * ((2.0f * p1) + (-p0 + p2) * t
            + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
            + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

// Draw Catmull-Rom spline through points array
static void
draw_catmull_rom_spline(framebuffer_t *fb,
                        const float *points_x,
                        const float *points_y,
                        int point_count,
                        uint32_t color,
                        float thickness)
{
  if (point_count < 2)
    return;

  if (point_count == 2)
    {
      // Just 2 points: straight line
      draw_line(fb, (int)points_x[0], (int)points_y[0], (int)points_x[1],
                (int)points_y[1], color, thickness);
      return;
    }

  // For each span between points[i] and points[i+1]
  for (int i = 0; i < point_count - 1; i++)
    {
      // Get 4 control points (duplicate endpoints for boundary)
      int i0 = (i > 0) ? i - 1 : 0;
      int i1 = i;
      int i2 = i + 1;
      int i3 = (i + 2 < point_count) ? i + 2 : point_count - 1;

      float x0 = points_x[i0], y0 = points_y[i0];
      float x1 = points_x[i1], y1 = points_y[i1];
      float x2 = points_x[i2], y2 = points_y[i2];
      float x3 = points_x[i3], y3 = points_y[i3];

      float prev_x = x1, prev_y = y1;

      for (int seg = 1; seg <= SPLINE_SEGMENTS_PER_SPAN; seg++)
        {
          float t      = (float)seg / SPLINE_SEGMENTS_PER_SPAN;
          float curr_x = catmull_rom(x0, x1, x2, x3, t);
          float curr_y = catmull_rom(y0, y1, y2, y3, t);

          draw_line(fb, (int)prev_x, (int)prev_y, (int)curr_x, (int)curr_y,
                    color, thickness);

          prev_x = curr_x;
          prev_y = curr_y;
        }
    }
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
// RENDER: 30-second history chart with Catmull-Rom spline
// ════════════════════════════════════════════════════════════

static void
render_history_chart(framebuffer_t *fb, int x, int y, uint64_t now_us)
{
  int chart_y = y + LABEL_HEIGHT;
  draw_rect_outline(fb, x, chart_y, CHART_WIDTH, BOX_SIZE, COLOR_BORDER, 1.0f);

  if (s_history_count == 0)
    return;

  uint64_t window_start
    = (now_us > HISTORY_WINDOW_US) ? (now_us - HISTORY_WINDOW_US) : 0;

  // ═══════════════════════════════════════════════════════════
  // Pass 1: Collect visible samples in TIME order, find min/max
  // ═══════════════════════════════════════════════════════════
  float min_s       = 1.0f;
  float max_s       = 0.0f;
  int visible_count = 0;

  // Stack arrays for visible samples (bounded by HISTORY_SIZE)
  float vis_sharpness[HISTORY_SIZE];
  float vis_smoothed[HISTORY_SIZE];
  uint64_t vis_time[HISTORY_SIZE];

  for (int n = 0; n < s_history_count; n++)
    {
      int idx = (s_history_head + n) % HISTORY_SIZE; // TIME ORDER!
      history_sample_t *sample = &s_history[idx];

      if (sample->time_us >= window_start && sample->valid)
        {
          vis_sharpness[visible_count] = sample->sharpness;
          vis_smoothed[visible_count]  = sample->smoothed;
          vis_time[visible_count]      = sample->time_us;

          if (sample->sharpness < min_s)
            min_s = sample->sharpness;
          if (sample->sharpness > max_s)
            max_s = sample->sharpness;
          visible_count++;
        }
    }

  if (visible_count == 0)
    return;

  float range = max_s - min_s;
  if (range < 0.001f)
    range = 1.0f;

  // ═══════════════════════════════════════════════════════════
  // Pass 2: Compute screen coordinates for all visible points
  // ═══════════════════════════════════════════════════════════
  float points_x[HISTORY_SIZE];
  float points_y[HISTORY_SIZE]; // Using smoothed values for curve

  for (int i = 0; i < visible_count; i++)
    {
      // TIME-BASED X position (0.0 = left edge = window_start, 1.0 = right =
      // now)
      float t = (float)(vis_time[i] - window_start) / (float)HISTORY_WINDOW_US;
      t       = fmaxf(0.0f, fminf(1.0f, t)); // Clamp for safety

      points_x[i] = (float)(x + 2) + t * (float)(CHART_WIDTH - 4);

      // Y position from smoothed value (for curve)
      float norm = (vis_smoothed[i] - min_s) / range;
      norm       = fmaxf(0.0f, fminf(1.0f, norm));
      points_y[i]
        = (float)(chart_y + BOX_SIZE - 2) - norm * (float)(BOX_SIZE - 4);
    }

  // ═══════════════════════════════════════════════════════════
  // Pass 3: Draw semi-transparent fill under curve
  // ═══════════════════════════════════════════════════════════
  int baseline_y = chart_y + BOX_SIZE - 2;
  for (int i = 0; i < visible_count; i++)
    {
      int px = (int)points_x[i];
      int py = (int)points_y[i];
      if (py < baseline_y)
        {
          draw_rect_filled(fb, px, py, 2, baseline_y - py, COLOR_CURVE_FILL);
        }
    }

  // ═══════════════════════════════════════════════════════════
  // Pass 4: Draw Catmull-Rom spline curve
  // ═══════════════════════════════════════════════════════════
  if (visible_count >= 2)
    {
      draw_catmull_rom_spline(fb, points_x, points_y, visible_count,
                              COLOR_CURVE, SPLINE_THICKNESS);
    }

  // ═══════════════════════════════════════════════════════════
  // Pass 5: Draw raw data points as small dots
  // ═══════════════════════════════════════════════════════════
  for (int i = 0; i < visible_count; i++)
    {
      // Raw sharpness Y position
      float norm = (vis_sharpness[i] - min_s) / range;
      norm       = fmaxf(0.0f, fminf(1.0f, norm));
      int raw_y
        = (int)((float)(chart_y + BOX_SIZE - 2) - norm * (float)(BOX_SIZE - 4));

      // Draw small dot for raw data
      draw_filled_circle(fb, (int)points_x[i], raw_y, 1.5f, COLOR_HISTORY_BAR);
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

  // Add to history with validation and EMA smoothing
  history_add_sample(sharp.global_score, now_us);

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
