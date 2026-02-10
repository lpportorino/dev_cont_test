/**
 * @file detections.c
 * @brief YOLO object detection overlay widget
 *
 * Renders bounding boxes with COCO-80 class labels for detected objects.
 * Detection data comes from ObjectDetectionsDay/Heat opaque payloads.
 * Only renders when status == DETECTION_STATUS_OK (1).
 */

#include "widgets/detections.h"

#include "core/framebuffer.h"
#include "osd_state.h"
#include "rendering/blending.h"
#include "rendering/primitives.h"
#include "rendering/text.h"
#include "utils/logging.h"

#include <stdio.h>

// Detection status OK value (ser_DetectionStatus_DETECTION_STATUS_OK)
#define DETECTION_STATUS_OK 1

// COCO 80-class labels
static const char *COCO_LABELS[80] = {
  "person",        "bicycle",      "car",
  "motorcycle",    "airplane",     "bus",
  "train",         "truck",        "boat",
  "traffic light", "fire hydrant", "stop sign",
  "parking meter", "bench",        "bird",
  "cat",           "dog",          "horse",
  "sheep",         "cow",          "elephant",
  "bear",          "zebra",        "giraffe",
  "backpack",      "umbrella",     "handbag",
  "tie",           "suitcase",     "frisbee",
  "skis",          "snowboard",    "sports ball",
  "kite",          "baseball bat", "baseball glove",
  "skateboard",    "surfboard",    "tennis racket",
  "bottle",        "wine glass",   "cup",
  "fork",          "knife",        "spoon",
  "bowl",          "banana",       "apple",
  "sandwich",      "orange",       "broccoli",
  "carrot",        "hot dog",      "pizza",
  "donut",         "cake",         "chair",
  "couch",         "potted plant", "bed",
  "dining table",  "toilet",       "tv",
  "laptop",        "mouse",        "remote",
  "keyboard",      "cell phone",   "microwave",
  "oven",          "toaster",      "sink",
  "refrigerator",  "book",         "clock",
  "vase",          "scissors",     "teddy bear",
  "hair drier",    "toothbrush",
};

// 8-color palette for per-class coloring (internal 0xAABBGGRR)
static const uint32_t CLASS_COLORS[8] = {
  0xFF00FF00, // Green
  0xFF00FFFF, // Yellow
  0xFFFF00FF, // Magenta
  0xFFFFFF00, // Cyan
  0xFF0000FF, // Red
  0xFFFF8000, // Light blue
  0xFF00FF80, // Yellow-green
  0xFF8000FF, // Pink
};

/**
 * Get display label for a COCO class ID
 */
static const char *
get_class_label(int class_id)
{
  if (class_id >= 0 && class_id < 80)
    {
      return COCO_LABELS[class_id];
    }
  return "?";
}

/**
 * Get color for a detection, either single color or per-class palette
 */
static uint32_t
get_detection_color(const detections_config_t *config, int class_id)
{
  if (config->per_class_color)
    {
      return CLASS_COLORS[class_id % 8];
    }
  return config->color;
}

bool
detections_render(osd_context_t *ctx, const osd_state_t *state)
{
  (void)state;

  if (!ctx->config.detections.enabled)
    {
      return false;
    }

  osd_detections_data_t data;
  if (!osd_state_get_detections(ctx, &data) || !data.valid)
    {
      return false;
    }

  // Only render when inference succeeded
  if (data.status != DETECTION_STATUS_OK)
    {
      return false;
    }

  if (data.count == 0)
    {
      return false;
    }

  framebuffer_t fb             = osd_ctx_get_framebuffer(ctx);
  const detections_config_t *c = &ctx->config.detections;
  float w                      = (float)ctx->width;
  float h                      = (float)ctx->height;
  bool rendered                = false;

  for (int i = 0; i < data.count; i++)
    {
      const osd_detection_t *det = &data.items[i];

      if (det->confidence < c->min_confidence)
        {
          continue;
        }

      // Convert NDC [-1.0, 1.0] to pixel coordinates
      int px1 = (int)((det->x1 + 1.0f) / 2.0f * w);
      int py1 = (int)((det->y1 + 1.0f) / 2.0f * h);
      int px2 = (int)((det->x2 + 1.0f) / 2.0f * w);
      int py2 = (int)((det->y2 + 1.0f) / 2.0f * h);

      int bw = px2 - px1;
      int bh = py2 - py1;
      if (bw <= 0 || bh <= 0)
        {
          continue;
        }

      uint32_t color = get_detection_color(c, det->class_id);

      // Draw bounding box
      draw_rect_outline(&fb, px1, py1, bw, bh, color, c->box_thickness);

      // Build label string
      char label[48];
      const char *class_name = get_class_label(det->class_id);
      snprintf(label, sizeof(label), "%s %.0f%%", class_name,
               det->confidence * 100.0f);

      // Measure label width for background
      int label_w = text_measure_width(&ctx->font_variant_info, label,
                                       c->label_font_size);
      int label_h = c->label_font_size + 2;

      // Label background (dark semi-transparent)
      int lx = px1;
      int ly = py1 - label_h - 1;
      if (ly < 0)
        ly = py1 + 1; // Below box if too close to top
      draw_rect_filled(&fb, lx, ly, label_w + 4, label_h, 0xA0000000);

      // Label text
      text_render_with_outline(&fb, &ctx->font_variant_info, label, lx + 2,
                               ly + 1, color, 0xFF000000, c->label_font_size,
                               1);

      rendered = true;
    }

  return rendered;
}
