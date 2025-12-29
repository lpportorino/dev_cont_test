// Timestamp Widget
// Provides timestamp overlay for OSD
//
// This module renders time-based information:
// - UTC timestamp (HH:MM:SS format)

#ifndef WIDGETS_TIMESTAMP_H
#define WIDGETS_TIMESTAMP_H

// ════════════════════════════════════════════════════════════
// WIDGET INCLUDES (minimal set for OSD development)
// ════════════════════════════════════════════════════════════
#include "core/osd_context.h" // osd_context_t (config, resources)

#include <stdbool.h>
#include <stdint.h>

// Forward declare state type (implementation uses osd_state.h accessors)
typedef struct _ser_JonGUIState osd_state_t;

// ════════════════════════════════════════════════════════════
// TIMESTAMP RENDERING
// ════════════════════════════════════════════════════════════

// Render UTC timestamp overlay
//
// Formats and renders current time from protobuf state as HH:MM:SS UTC.
// Renders with black outline for visibility on any background.
//
// Parameters:
//   ctx:      OSD context (for font, config, and text rendering)
//   pb_state: Telemetry state containing timestamp
//
// Returns:
//   true if timestamp was rendered, false if disabled or no time data
//
// Example:
//   bool changed = timestamp_render(ctx, pb_state);
//
// Notes:
//   - Only renders if ctx->config.timestamp.enabled is true
//   - Requires pb_state->has_time to be true
//   - Uses ctx->config.timestamp.pos_x/pos_y for position
//   - Uses ctx->config.timestamp.color and font_size
//   - Renders with 2px black outline for visibility
//   - Hidden in live mode (OSD_SHOW_TIMESTAMP = 0)
bool timestamp_render(osd_context_t *ctx, osd_state_t *pb_state);

#endif // WIDGETS_TIMESTAMP_H
