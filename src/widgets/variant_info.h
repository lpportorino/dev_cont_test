// Variant Info Widget
// Displays current WASM variant and configuration values
//
// Useful for debugging and demonstrating compile-time vs runtime config.

#ifndef VARIANT_INFO_H
#define VARIANT_INFO_H

// ════════════════════════════════════════════════════════════
// WIDGET INCLUDES (minimal set for OSD development)
// ════════════════════════════════════════════════════════════
#include "core/osd_context.h" // osd_context_t (config, resources)

#include <stdbool.h>

// Forward declare state type (implementation uses osd_state.h accessors)
typedef struct _ser_JonGUIState osd_state_t;

// ════════════════════════════════════════════════════════════
// VARIANT INFO WIDGET API
// ════════════════════════════════════════════════════════════

// Initialize variant info widget
//
// Parameters:
//   ctx: OSD context
void variant_info_init(osd_context_t *ctx);

// Render variant info widget
//
// Shows:
// - Variant name (from compile-time defines)
// - Active configuration values
//
// Parameters:
//   ctx:   OSD context
//   state: Telemetry state (unused, for API consistency with other widgets)
//
// Returns:
//   true if variant info was rendered, false otherwise
bool variant_info_render(osd_context_t *ctx, const osd_state_t *state);

// Cleanup variant info widget
//
// Parameters:
//   ctx: OSD context
void variant_info_cleanup(osd_context_t *ctx);

#endif // VARIANT_INFO_H
