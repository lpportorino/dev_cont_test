// YOLO Object Detections Overlay Widget
// Renders bounding boxes with COCO-80 class labels for detected objects
// from ObjectDetectionsDay/Heat opaque payloads.

#ifndef WIDGETS_DETECTIONS_H
#define WIDGETS_DETECTIONS_H

#include "core/osd_context.h"

#include <stdbool.h>

// Forward declare state type
typedef struct _ser_JonGUIState osd_state_t;

// Render detection overlay widget
// Returns true if rendered, false if disabled or no data
bool detections_render(osd_context_t *ctx, const osd_state_t *state);

#endif // WIDGETS_DETECTIONS_H
