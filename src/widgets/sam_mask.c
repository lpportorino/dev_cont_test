/**
 * @file sam_mask.c
 * @brief SAM visual tracking overlay widget
 *
 * Renders bounding box, centroid marker, and state indicator for tracked
 * objects. Tracking data comes from SamTrackingDay/Heat opaque payloads. Only
 * renders when status == SAM_TRACKING_STATUS_OK (1) and state == TRACKING.
 */

#include "widgets/sam_mask.h"

#include "core/framebuffer.h"
#include "core/osd_context.h"
#include "osd_state.h"
#include "rendering/blending.h"
#include "rendering/primitives.h"
#include "rendering/text.h"
#include "utils/logging.h"

#include <stdio.h>

// State colors (internal 0xAABBGGRR format)
#define SAM_COLOR_TRACKING 0xFF00FF00 // Green - normal tracking
#define SAM_COLOR_OCCLUDED 0xFF00FFFF // Yellow - occluded/low confidence
#define SAM_COLOR_STARTING 0xFFFFFF00 // Cyan - starting up
#define SAM_COLOR_LOST 0xFF0000FF     // Red - lost

/**
 * Get color for tracking state
 */
static uint32_t
get_state_color(int state, const sam_mask_config_t *config)
{
  // If per-state coloring disabled, use configured color
  if (!config->per_state_color)
    {
      return config->color;
    }

  switch (state)
    {
    case OSD_SAM_STATE_TRACKING:
      return SAM_COLOR_TRACKING;
    case OSD_SAM_STATE_OCCLUDED:
      return SAM_COLOR_OCCLUDED;
    case OSD_SAM_STATE_STARTING:
      return SAM_COLOR_STARTING;
    case OSD_SAM_STATE_LOST:
      return SAM_COLOR_LOST;
    default:
      return config->color;
    }
}

/**
 * Decode RLE mask data.
 * Format: [run_length:u16, value:u8]... (little-endian)
 *
 * @param rle_data Raw RLE bytes
 * @param rle_len Length of RLE data
 * @param out_data Output buffer (must be width * height bytes)
 * @param width Mask width
 * @param height Mask height
 * @return true on success
 */
static bool
decode_rle_mask(const uint8_t *rle_data,
                size_t rle_len,
                uint8_t *out_data,
                uint32_t width,
                uint32_t height)
{
  size_t total_pixels = (size_t)width * height;
  size_t pixel_idx    = 0;
  size_t rle_idx      = 0;

  while (rle_idx + 2 < rle_len && pixel_idx < total_pixels)
    {
      // Read run_length (u16 little-endian)
      uint16_t run_length
        = (uint16_t)rle_data[rle_idx] | ((uint16_t)rle_data[rle_idx + 1] << 8);
      rle_idx += 2;

      // Read value (u8)
      uint8_t value = rle_data[rle_idx++];

      // Write run
      size_t end = pixel_idx + run_length;
      if (end > total_pixels)
        {
          end = total_pixels;
        }

      while (pixel_idx < end)
        {
          out_data[pixel_idx++] = value;
        }
    }

  return pixel_idx == total_pixels;
}

/**
 * Render binary mask as semi-transparent overlay.
 *
 * The SAM 256x256 mask represents a 512x512 CENTER CROP from the full frame,
 * NOT the full frame. The crop is centered at ((width-512)/2, (height-512)/2).
 *
 * Coordinate transformation:
 *   mask (256x256) → crop (512x512) → frame (1920x1080)
 *   Scale: 2x (mask to crop)
 *   Offset: (704, 284) for 1920x1080
 *
 * @param fb        Framebuffer to render to
 * @param mask      Binary mask data (256x256)
 * @param mask_w    Mask width
 * @param mask_h    Mask height
 * @param color     Mask color (AABBGGRR)
 * @param alpha     Mask alpha (0-255)
 */
static void
render_mask_overlay(framebuffer_t *fb,
                    const uint8_t *mask,
                    uint32_t mask_w,
                    uint32_t mask_h,
                    uint32_t color,
                    uint8_t alpha)
{
  // SAM uses 512x512 center crop from input frame
  const int crop_size = 512;
  int crop_x          = ((int)fb->width - crop_size) / 2;  // 704 for 1920
  int crop_y          = ((int)fb->height - crop_size) / 2; // 284 for 1080

  // Scale factor: mask (256) → crop (512) = 2.0
  float scale = (float)crop_size / (float)mask_w;

  // Blend color with alpha
  uint32_t blend_color = (color & 0x00FFFFFF) | ((uint32_t)alpha << 24);

  for (uint32_t my = 0; my < mask_h; my++)
    {
      for (uint32_t mx = 0; mx < mask_w; mx++)
        {
          if (mask[my * mask_w + mx])
            {
              // Scale mask coords to crop space, then offset to frame space
              int sx     = crop_x + (int)((float)mx * scale);
              int sy     = crop_y + (int)((float)my * scale);
              int sx_end = crop_x + (int)((float)(mx + 1) * scale);
              int sy_end = crop_y + (int)((float)(my + 1) * scale);

              // Draw scaled pixel (fill the scaled region)
              for (int py = sy; py < sy_end; py++)
                {
                  for (int px = sx; px < sx_end; px++)
                    {
                      framebuffer_blend_pixel(fb, px, py, blend_color);
                    }
                }
            }
        }
    }
}

/**
 * Get state name for label
 */
static const char *
get_state_name(int state)
{
  switch (state)
    {
    case OSD_SAM_STATE_IDLE:
      return "IDLE";
    case OSD_SAM_STATE_STARTING:
      return "STARTING";
    case OSD_SAM_STATE_TRACKING:
      return "TRACKING";
    case OSD_SAM_STATE_OCCLUDED:
      return "OCCLUDED";
    case OSD_SAM_STATE_LOST:
      return "LOST";
    default:
      return "?";
    }
}

bool
sam_mask_render(osd_context_t *ctx, const osd_state_t *state)
{
  (void)state;

  if (!ctx->config.sam_mask.enabled)
    {
      return false;
    }

  osd_sam_tracking_data_t data;
  if (!osd_state_get_sam_tracking(ctx, &data) || !data.valid)
    {
      return false;
    }

  // Only render when tracking is OK and we have valid state
  if (data.status != OSD_SAM_STATUS_OK)
    {
      return false;
    }

  // Don't render if idle (no tracking active)
  if (data.state == OSD_SAM_STATE_IDLE)
    {
      return false;
    }

  framebuffer_t fb           = osd_ctx_get_framebuffer(ctx);
  const sam_mask_config_t *c = &ctx->config.sam_mask;
  float w                    = (float)ctx->width;
  float h                    = (float)ctx->height;
  bool rendered              = false;

  // Get color based on state
  uint32_t color = get_state_color(data.state, c);

  // Convert NDC [-1.0, 1.0] to pixel coordinates
  // Note: SAM uses image-space NDC where y1=top (smaller Y = top)
  int px1 = (int)((data.bbox_x1 + 1.0f) / 2.0f * w);
  int py1 = (int)((data.bbox_y1 + 1.0f) / 2.0f * h);
  int px2 = (int)((data.bbox_x2 + 1.0f) / 2.0f * w);
  int py2 = (int)((data.bbox_y2 + 1.0f) / 2.0f * h);

  int bw = px2 - px1;
  int bh = py2 - py1;
  if (bw <= 0 || bh <= 0)
    {
      return false;
    }

  // Draw bounding box
  draw_rect_outline(&fb, px1, py1, bw, bh, color, c->box_thickness);
  rendered = true;

  // Render mask overlay if enabled and data available
  if (c->mask_enabled && ctx->sam_tracking.mask_rle_len > 0
      && data.mask_width > 0 && data.mask_height > 0
      && data.mask_width <= OSD_SAM_MASK_WIDTH
      && data.mask_height <= OSD_SAM_MASK_HEIGHT)
    {
      // Decode RLE into static buffer
      if (decode_rle_mask(
            ctx->sam_tracking.mask_rle, ctx->sam_tracking.mask_rle_len,
            ctx->sam_tracking.mask_data, data.mask_width, data.mask_height))
        {
          // Render mask at full frame scale (mask is 256x256 for full frame)
          render_mask_overlay(&fb, ctx->sam_tracking.mask_data, data.mask_width,
                              data.mask_height, color, c->mask_alpha);
        }
    }

  // Draw centroid marker (image-space NDC: smaller Y = top)
  int cx     = (int)((data.centroid_x + 1.0f) / 2.0f * w);
  int cy     = (int)((data.centroid_y + 1.0f) / 2.0f * h);
  int radius = c->centroid_radius;

  // Draw crosshair at centroid
  draw_line(&fb, cx - radius, cy, cx + radius, cy, color, 2.0f);
  draw_line(&fb, cx, cy - radius, cx, cy + radius, color, 2.0f);

  // Draw Kalman prediction marker (if different from centroid)
  // Image-space NDC: smaller Y = top
  if (data.kf_predicted_x != 0.0f || data.kf_predicted_y != 0.0f)
    {
      int kx = (int)((data.kf_predicted_x + 1.0f) / 2.0f * w);
      int ky = (int)((data.kf_predicted_y + 1.0f) / 2.0f * h);

      // Draw small hollow circle for Kalman prediction
      if (kx != cx || ky != cy)
        {
          // Draw small X marker for prediction
          uint32_t pred_color = 0x80FFFFFF; // Semi-transparent white
          draw_line(&fb, kx - 3, ky - 3, kx + 3, ky + 3, pred_color, 1.0f);
          draw_line(&fb, kx - 3, ky + 3, kx + 3, ky - 3, pred_color, 1.0f);
        }
    }

  // Build label string with state and confidence
  char label[64];
  const char *state_name = get_state_name(data.state);
  snprintf(label, sizeof(label), "%s %.0f%%", state_name,
           data.confidence * 100.0f);

  // Measure label width for background
  int label_w
    = text_measure_width(&ctx->font_variant_info, label, c->label_font_size);
  int label_h = c->label_font_size + 2;

  // Label background (dark semi-transparent)
  int lx = px1;
  int ly = py1 - label_h - 1;
  if (ly < 0)
    ly = py1 + 1; // Below box if too close to top
  draw_rect_filled(&fb, lx, ly, label_w + 4, label_h, 0xA0000000);

  // Label text
  text_render_with_outline(&fb, &ctx->font_variant_info, label, lx + 2, ly + 1,
                           color, 0xFF000000, c->label_font_size, 1);

  // Show lost frame count if in LOST or OCCLUDED state
  if ((data.state == OSD_SAM_STATE_LOST || data.state == OSD_SAM_STATE_OCCLUDED)
      && data.lost_frame_count > 0)
    {
      char lost_label[32];
      snprintf(lost_label, sizeof(lost_label), "Lost: %u",
               data.lost_frame_count);
      int lost_w = text_measure_width(&ctx->font_variant_info, lost_label,
                                      c->label_font_size - 2);
      int lost_x = px2 - lost_w - 4;
      int lost_y = ly;
      draw_rect_filled(&fb, lost_x, lost_y, lost_w + 4, label_h - 2,
                       0x80000000);
      text_render_with_outline(&fb, &ctx->font_variant_info, lost_label,
                               lost_x + 2, lost_y + 1, 0xFFFFFFFF, 0xFF000000,
                               c->label_font_size - 2, 1);
    }

  return rendered;
}
