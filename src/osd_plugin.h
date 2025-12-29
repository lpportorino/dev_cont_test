// OSD Plugin Header
// Main header for OSD plugin implementation
//
// ════════════════════════════════════════════════════════════
// HEADER ORGANIZATION:
//
// For WIDGET DEVELOPERS (rendering OSD elements):
//   #include "core/osd_context.h"  - Context type and helpers
//   #include "osd_state.h"         - State accessors (orientation, speeds,
//   etc.) #include "rendering/primitives.h" - Drawing functions
//
// For WASM INFRASTRUCTURE (host boundary):
//   #include "wasm/wasm_exports.h" - WASM export macros and API
//
// This header (osd_plugin.h) is for the main plugin implementation only.
// Widgets should NOT include this header directly.
// ════════════════════════════════════════════════════════════

#ifndef OSD_PLUGIN_H
#define OSD_PLUGIN_H

// Include the modular headers
#include "core/osd_context.h"
#include "wasm/wasm_exports.h"

// Forward declarations
typedef struct _ser_JonGUIState ser_JonGUIState;

// ════════════════════════════════════════════════════════════
// INTERNAL RENDERING FUNCTIONS
// ════════════════════════════════════════════════════════════
//
// These functions are used internally by osd_plugin.c for rendering.
// Widgets should use the primitives in rendering/primitives.h instead.

// Text rendering (uses font resource from context)
void render_text(osd_context_t *ctx,
                 const char *text,
                 int x,
                 int y,
                 uint32_t color,
                 int font_size);

void render_text_with_outline(osd_context_t *ctx,
                              const char *text,
                              int x,
                              int y,
                              uint32_t color,
                              uint32_t outline_color,
                              int font_size,
                              int outline_thickness);

// Timestamp rendering (recording builds only)
void render_timestamp(osd_context_t *ctx, ser_JonGUIState *pb_state);

// Configuration loading (JSON-based)
bool load_config_xml(osd_context_t *ctx, const char *path);

// Proto decoding (internal use only)
bool decode_proto_state(osd_context_t *ctx, ser_JonGUIState *pb_state);

#endif // OSD_PLUGIN_H
