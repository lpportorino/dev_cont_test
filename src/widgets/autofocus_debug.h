/**
 * @file autofocus_debug.h
 * @brief Autofocus debug panel widget
 *
 * Unified debugging panel for autofocus system showing:
 * - Focus and zoom lens position indicators (vertical bars)
 * - 8x8 sharpness heatmap grid
 * - Focus-to-sharpness mapping scatter plot (resets on zoom/rotary change)
 * - 10-second sharpness history chart
 */

#ifndef WIDGETS_AUTOFOCUS_DEBUG_H
#define WIDGETS_AUTOFOCUS_DEBUG_H

#include "core/osd_context.h"

#include <stdbool.h>
#include <stdint.h>

// Forward declare state type (implementation uses osd_state.h accessors)
typedef struct _ser_JonGUIState osd_state_t;

/**
 * Render the autofocus debug panel.
 *
 * Displays lens positions, sharpness heatmap, focus-to-sharpness mapping,
 * and 10-second sharpness history in a unified panel.
 *
 * @param ctx      OSD context (for config, font, framebuffer, cv_meta)
 * @param pb_state Telemetry state (for camera_day, rotary, monotonic time)
 *
 * @return true if widget was rendered, false if disabled or no data
 */
bool autofocus_debug_render(osd_context_t *ctx, osd_state_t *pb_state);

#endif // WIDGETS_AUTOFOCUS_DEBUG_H
