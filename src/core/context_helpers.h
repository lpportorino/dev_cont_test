// Context Helper Functions
// Utility functions for working with OSD context
//
// NOTE: Most helpers have been moved into osd_context.h as static inline.
// This file remains for backward compatibility.

#ifndef CONTEXT_HELPERS_H
#define CONTEXT_HELPERS_H

#include "osd_context.h"

// ctx_to_framebuffer() is now osd_ctx_get_framebuffer() in osd_context.h
// This alias maintains backward compatibility with existing widget code.
static inline framebuffer_t
ctx_to_framebuffer(osd_context_t *ctx)
{
  return osd_ctx_get_framebuffer(ctx);
}

#endif // CONTEXT_HELPERS_H
