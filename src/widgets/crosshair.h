// Crosshair Widget
// Provides crosshair overlay rendering for targeting and alignment
//
// This module renders customizable crosshair overlays with multiple elements:
// - Center dot (filled circle)
// - Cross arms (+ or X shape with configurable gap and length)
// - Circle outline (ring around center)

#ifndef WIDGETS_CROSSHAIR_H
#define WIDGETS_CROSSHAIR_H

// ════════════════════════════════════════════════════════════
// WIDGET INCLUDES (minimal set for OSD development)
// ════════════════════════════════════════════════════════════
#include "config/osd_config.h" // crosshair_config_t
#include "core/framebuffer.h"  // framebuffer_t for rendering
#include "core/osd_context.h"  // osd_context_t (config, resources)

#include <stdbool.h>
#include <stdint.h>

// ════════════════════════════════════════════════════════════
// CROSSHAIR RENDERING
// ════════════════════════════════════════════════════════════

// Forward declare state type (implementation uses osd_state.h accessors)
typedef struct _ser_JonGUIState osd_state_t;

// Render complete crosshair overlay with speed indicators
//
// Renders all enabled crosshair elements (circle, cross, center dot)
// and speed indicators radially positioned around the crosshair.
//
// Parameters:
//   ctx:      OSD context (for config, font, framebuffer dimensions)
//   pb_state: Telemetry state (for rotary speed data)
//
// Returns:
//   true if crosshair was rendered, false if disabled or nothing to draw
//
// Example:
//   bool changed = crosshair_render(&g_osd_ctx, &pb_state);
//
// Notes:
//   - Respects config.crosshair.enabled flag
//   - Applies offset_x/offset_y from center
//   - Renders in order: circle -> cross -> center dot -> speed indicators
//   - Speed indicators positioned radially around crosshair (web version style)
bool crosshair_render(osd_context_t *ctx, osd_state_t *pb_state);

// Render crosshair center dot
//
// Renders a filled circle at the crosshair center point.
//
// Parameters:
//   fb:     Framebuffer to render into
//   config: Crosshair configuration
//   cx, cy: Center coordinates
//
// Notes:
//   - Only renders if config->center_dot.enabled is true
//   - Uses config->center_dot_radius for size
//   - Uses config->center_dot.color (ARGB format)
void crosshair_render_center_dot(framebuffer_t *fb,
                                 const crosshair_config_t *config,
                                 int cx,
                                 int cy);

// Render crosshair cross arms
//
// Renders 4 lines extending from the center with a gap.
// Supports two orientations:
// - VERTICAL: + shape (0, 90, 180, 270 degrees)
// - DIAGONAL: X shape (45, 135, 225, 315 degrees)
//
// Parameters:
//   fb:     Framebuffer to render into
//   config: Crosshair configuration
//   cx, cy: Center coordinates
//
// Notes:
//   - Only renders if config->cross.enabled is true
//   - Uses config->cross_gap (distance from center)
//   - Uses config->cross_length (arm length)
//   - Uses config->cross.color and config->cross.thickness
//   - Diagonal lines scaled by cos(45) = 0.707
void crosshair_render_cross(framebuffer_t *fb,
                            const crosshair_config_t *config,
                            int cx,
                            int cy);

// Render crosshair circle outline
//
// Renders a circle outline (ring) around the crosshair center.
//
// Parameters:
//   fb:     Framebuffer to render into
//   config: Crosshair configuration
//   cx, cy: Center coordinates
//
// Notes:
//   - Only renders if config->circle.enabled is true
//   - Uses config->circle_radius for size
//   - Uses config->circle.color and config->circle.thickness
void crosshair_render_circle(framebuffer_t *fb,
                             const crosshair_config_t *config,
                             int cx,
                             int cy);

#endif // WIDGETS_CROSSHAIR_H
