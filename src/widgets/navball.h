// Nav Ball Widget
// Provides KSP-style nav ball rendering with orientation display
//
// This module renders a 3D textured sphere (nav ball) that rotates based on
// compass/heading data (azimuth, elevation, bank) from the proto message.
// Includes a fixed level marker overlay (KSP-style with shadow).

#ifndef WIDGETS_NAVBALL_H
#define WIDGETS_NAVBALL_H

// ════════════════════════════════════════════════════════════
// WIDGET INCLUDES (minimal set for OSD development)
// ════════════════════════════════════════════════════════════
#include "config/osd_config.h" // navball_config_t
#include "core/osd_context.h" // osd_context_t, navball_config_t, navball_skin_t

#include <stdbool.h>

// Forward declare state type (implementation uses osd_state.h accessors)
typedef struct _ser_JonGUIState osd_state_t;

// ════════════════════════════════════════════════════════════
// NAV BALL WIDGET API
// ════════════════════════════════════════════════════════════

// Initialize nav ball widget
//
// Loads the configured skin texture and initializes rendering resources.
//
// Parameters:
//   ctx:    OSD context
//   config: Nav ball configuration from JSON
//
// Notes:
//   - Loads skin PNG from resources/navball_skins/
//   - Pre-rasterizes level marker SVG
//   - Allocates temporary rendering buffers
//
// Example:
//   navball_config_t config = {
//     .enabled = true,
//     .position_x = 100,
//     .position_y = 100,
//     .size = 256,
//     .skin = NAVBALL_SKIN_STOCK,
//     .show_level_marker = true
//   };
//   navball_init(ctx, &config);
//
// Note: Nav ball always uses LUT precomputation and 16.16 fixed-point math
// for optimal WASM performance (no configuration needed).
bool navball_init(osd_context_t *ctx, const navball_config_t *config);

// Render nav ball widget
//
// Renders the nav ball at the configured screen position with rotation
// based on compass data (azimuth/elevation/bank) from the proto message.
//
// Parameters:
//   ctx:      OSD context (contains framebuffer and nav ball state)
//   pb_state: Telemetry state containing compass data
//
// Returns:
//   true if navball was rendered, false if disabled or no texture
//
// Rendering Process:
//   1. Extract compass data (azimuth, elevation, bank)
//   2. Clear nav ball region to background color
//   3. For each pixel in the nav ball circle:
//      a. Compute 3D point on sphere surface
//      b. Apply rotation matrix (pitch/yaw/roll)
//      c. Convert rotated 3D point to texture UV coordinates
//      d. Sample skin texture
//      e. Blend pixel to framebuffer
//   4. Render level marker overlay (fixed screen position)
//
// Example:
//   bool changed = navball_render(ctx, pb_state);
//
// Notes:
//   - Requires pb_state->has_compass to be true
//   - Level marker is rendered on top (fixed orientation)
//   - Uses alpha blending for anti-aliasing
//   - Nav ball rotation: pitch=elevation, yaw=azimuth, roll=bank
bool navball_render(osd_context_t *ctx, osd_state_t *pb_state);

// Cleanup nav ball resources
//
// Frees allocated textures and rendering buffers.
//
// Parameters:
//   ctx: OSD context
void navball_cleanup(osd_context_t *ctx);

// ════════════════════════════════════════════════════════════
// SKIN UTILITIES
// ════════════════════════════════════════════════════════════

// Get skin filename from enum
//
// Returns the PNG filename for the given skin type.
//
// Parameters:
//   skin: Skin enum value
//
// Returns:
//   Static string with filename (e.g., "stock.png")
//   Returns "stock.png" for invalid skin values
const char *navball_skin_to_filename(navball_skin_t skin);

// Parse skin from string (for JSON loading)
//
// Converts skin name string to enum value.
//
// Parameters:
//   name: Skin name (e.g., "stock", "jafo", "apollo")
//
// Returns:
//   Matching enum value, or NAVBALL_SKIN_STOCK if not found
//
// Example:
//   navball_skin_t skin = navball_skin_from_string("apollo");
//   // Returns NAVBALL_SKIN_APOLLO
navball_skin_t navball_skin_from_string(const char *name);

#endif // WIDGETS_NAVBALL_H
