// Sharpness Heatmap Widget
// Renders an 8x8 color-coded grid showing per-region sharpness scores
// from the CvMeta opaque payload sharpness pyramid level 3.

#ifndef WIDGETS_SHARPNESS_HEATMAP_H
#define WIDGETS_SHARPNESS_HEATMAP_H

#include "core/osd_context.h"

#include <stdbool.h>

// Forward declare state type
typedef struct _ser_JonGUIState osd_state_t;

// Render sharpness heatmap widget
// Returns true if rendered, false if disabled or no data
bool sharpness_heatmap_render(osd_context_t *ctx, const osd_state_t *state);

#endif // WIDGETS_SHARPNESS_HEATMAP_H
