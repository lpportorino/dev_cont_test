// SAM Visual Tracking Overlay Widget
// Renders bounding box, centroid marker, and state indicator for SAM tracking
// from SamTrackingDay/Heat opaque payloads.

#ifndef WIDGETS_SAM_MASK_H
#define WIDGETS_SAM_MASK_H

#include "core/osd_context.h"

#include <stdbool.h>

// Forward declare state type
typedef struct _ser_JonGUIState osd_state_t;

// Render SAM tracking overlay widget
// Returns true if rendered, false if disabled or no tracking data
bool sam_mask_render(osd_context_t *ctx, const osd_state_t *state);

#endif // WIDGETS_SAM_MASK_H
