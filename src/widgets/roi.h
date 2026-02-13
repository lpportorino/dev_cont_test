// ROI Overlay Widget
// Renders colored bounding boxes for active ROI regions (focus, track, zoom,
// fx) from JonGuiDataCV proto state fields.

#ifndef WIDGETS_ROI_H
#define WIDGETS_ROI_H

#include "core/osd_context.h"

#include <stdbool.h>

// Forward declare state type
typedef struct _ser_JonGUIState osd_state_t;

// Render ROI overlay widget
// Returns true if rendered, false if disabled or no data
bool roi_render(osd_context_t *ctx, const osd_state_t *state);

#endif // WIDGETS_ROI_H
