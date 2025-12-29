// Logging System
// Provides structured logging with levels for OSD debugging and diagnostics
//
// This module replaces scattered fprintf(stderr, ...) calls with a
// centralized logging system that supports log levels and consistent
// formatting.

#ifndef UTILS_LOGGING_H
#define UTILS_LOGGING_H

#include <stdarg.h>
#include <stdbool.h>

// ════════════════════════════════════════════════════════════
// LOG LEVELS
// ════════════════════════════════════════════════════════════

typedef enum
{
  LOG_LEVEL_DEBUG = 0, // Verbose debugging information
  LOG_LEVEL_INFO  = 1, // General informational messages
  LOG_LEVEL_WARN  = 2, // Warning messages (potential issues)
  LOG_LEVEL_ERROR = 3, // Error messages (failures)
  LOG_LEVEL_NONE  = 4  // Disable all logging
} log_level_t;

// ════════════════════════════════════════════════════════════
// LOGGING CONFIGURATION
// ════════════════════════════════════════════════════════════

// Set minimum log level (messages below this level are suppressed)
//
// Default: LOG_LEVEL_INFO
//
// Example:
//   log_set_level(LOG_LEVEL_DEBUG);  // Show all messages
//   log_set_level(LOG_LEVEL_ERROR);  // Only show errors
//   log_set_level(LOG_LEVEL_NONE);   // Disable all logging
void log_set_level(log_level_t level);

// Get current minimum log level
log_level_t log_get_level(void);

// ════════════════════════════════════════════════════════════
// LOGGING FUNCTIONS
// ════════════════════════════════════════════════════════════

// Log a message with specified level
//
// Parameters:
//   level:  Log level for this message
//   format: printf-style format string
//   ...:    Format arguments
//
// Output format: "[LEVEL] message\n"
//
// Example:
//   log_message(LOG_LEVEL_INFO, "Initialized OSD: %dx%d", width, height);
//   // Output: "[INFO] Initialized OSD: 1920x1080\n"
void log_message(log_level_t level, const char *format, ...)
  __attribute__((format(printf, 2, 3)));

// Convenience macros for common log levels
//
// Usage:
//   LOG_DEBUG("Font size: %ld bytes", font_size);
//   LOG_INFO("Config loaded (using defaults)");
//   LOG_WARN("Font loading failed, text rendering disabled");
//   LOG_ERROR("Failed to parse SVG file: %s", path);
//
// Build mode behavior:
//   Production (NDEBUG defined): LOG_DEBUG and LOG_INFO compile to nothing
//   Development: All log levels active

#ifdef NDEBUG
// Production: compile away debug/info logs entirely (like assert())
#define LOG_DEBUG(...) ((void)0)
#define LOG_INFO(...) ((void)0)
#else
// Development: all logs active
#define LOG_DEBUG(...) log_message(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_INFO(...) log_message(LOG_LEVEL_INFO, __VA_ARGS__)
#endif

// Warnings and errors always active (both builds)
#define LOG_WARN(...) log_message(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_LEVEL_ERROR, __VA_ARGS__)

// ════════════════════════════════════════════════════════════
// CONDITIONAL LOGGING
// ════════════════════════════════════════════════════════════

// Check if a log level is enabled
//
// Useful for avoiding expensive operations when logging is disabled:
//
// Example:
//   if (log_is_enabled(LOG_LEVEL_DEBUG)) {
//     // Only compute expensive debug info if debug logging is on
//     char *debug_str = expensive_computation();
//     LOG_DEBUG("Debug info: %s", debug_str);
//     free(debug_str);
//   }
//
// Note: In production builds (NDEBUG), DEBUG and INFO always return false
static inline bool
log_is_enabled(log_level_t level)
{
#ifdef NDEBUG
  // Production: DEBUG and INFO are compiled out
  if (level <= LOG_LEVEL_INFO)
    {
      return false;
    }
#endif
  return level >= log_get_level();
}

// ════════════════════════════════════════════════════════════
// HELPER MACROS
// ════════════════════════════════════════════════════════════

// Log with function name prefix (useful for debugging)
//
// Example:
//   LOG_FUNC_DEBUG("Rendering frame %d", frame_num);
//   // Output: "[DEBUG] wasm_osd_render: Rendering frame 42\n"

#ifdef NDEBUG
// Production: compile away debug/info logs entirely
#define LOG_FUNC_DEBUG(...) ((void)0)
#define LOG_FUNC_INFO(...) ((void)0)
#else
// Development: all logs active
#define LOG_FUNC_DEBUG(...) \
  log_message(LOG_LEVEL_DEBUG, "%s: " __VA_ARGS__, __func__)
#define LOG_FUNC_INFO(...) \
  log_message(LOG_LEVEL_INFO, "%s: " __VA_ARGS__, __func__)
#endif

// Warnings and errors always active (both builds)
#define LOG_FUNC_WARN(...) \
  log_message(LOG_LEVEL_WARN, "%s: " __VA_ARGS__, __func__)
#define LOG_FUNC_ERROR(...) \
  log_message(LOG_LEVEL_ERROR, "%s: " __VA_ARGS__, __func__)

#endif // UTILS_LOGGING_H
