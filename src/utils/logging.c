#include "utils/logging.h"

#include <stdarg.h>
#include <stdio.h>

// ════════════════════════════════════════════════════════════
// GLOBAL STATE
// ════════════════════════════════════════════════════════════

static log_level_t g_min_log_level = LOG_LEVEL_INFO;

// ════════════════════════════════════════════════════════════
// CONFIGURATION IMPLEMENTATION
// ════════════════════════════════════════════════════════════

void
log_set_level(log_level_t level)
{
  g_min_log_level = level;
}

log_level_t
log_get_level(void)
{
  return g_min_log_level;
}

// ════════════════════════════════════════════════════════════
// LOGGING IMPLEMENTATION
// ════════════════════════════════════════════════════════════

// Get string representation of log level
static const char *
log_level_string(log_level_t level)
{
  switch (level)
    {
    case LOG_LEVEL_DEBUG:
      return "DEBUG";
    case LOG_LEVEL_INFO:
      return "INFO";
    case LOG_LEVEL_WARN:
      return "WARN";
    case LOG_LEVEL_ERROR:
      return "ERROR";
    case LOG_LEVEL_NONE:
      return "NONE";
    default:
      return "UNKNOWN";
    }
}

void
log_message(log_level_t level, const char *format, ...)
{
  // Check if this message should be logged
  if (level < g_min_log_level)
    {
      return;
    }

  // Print level prefix
  fprintf(stderr, "[%s] ", log_level_string(level));

  // Print formatted message
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  // Ensure newline
  fprintf(stderr, "\n");
}
