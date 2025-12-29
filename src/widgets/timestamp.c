#include "widgets/timestamp.h"

#include "core/context_helpers.h"
#include "core/framebuffer.h"
#include "jon_shared_data.pb.h"
#include "rendering/text.h"

#include <stdio.h>
#include <time.h>

// Outline thickness for timestamp text
#define TIMESTAMP_OUTLINE_THICKNESS 2

// ════════════════════════════════════════════════════════════
// TIMESTAMP RENDERING IMPLEMENTATION
// ════════════════════════════════════════════════════════════

bool
timestamp_render(osd_context_t *ctx, osd_state_t *pb_state)
{
  if (!ctx->config.timestamp.enabled)
    return false;
  if (!pb_state || !pb_state->has_time)
    return false;

  // Format timestamp as HH:MM:SS
  int64_t timestamp  = pb_state->time.timestamp;
  time_t t           = (time_t)timestamp;
  struct tm *tm_info = gmtime(&t);

  char time_str[32];
  snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d UTC", tm_info->tm_hour,
           tm_info->tm_min, tm_info->tm_sec);

  // Render with black outline for better visibility
  framebuffer_t fb = ctx_to_framebuffer(ctx);
  text_render_with_outline(
    &fb, &ctx->font_timestamp, time_str, ctx->config.timestamp.pos_x,
    ctx->config.timestamp.pos_y, ctx->config.timestamp.color,
    0xFF000000, // Black outline
    ctx->config.timestamp.font_size, TIMESTAMP_OUTLINE_THICKNESS);

  return true;
}
