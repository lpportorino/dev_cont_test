// Font Resource Management
// Provides font loading and initialization for OSD text rendering
//
// This module handles TrueType font loading using stb_truetype,
// managing font data buffers and font info structures.

#ifndef RESOURCES_FONT_H
#define RESOURCES_FONT_H

#include <stdbool.h>
#include <stddef.h>

// Forward declare stbtt_fontinfo to avoid including stb_truetype.h in header
typedef struct stbtt_fontinfo stbtt_fontinfo;

// ════════════════════════════════════════════════════════════
// FONT RESOURCE STRUCTURE
// ════════════════════════════════════════════════════════════

// Font resource handle
//
// Manages font file data and stb_truetype font info.
// Created by font_load(), destroyed by font_free().
typedef struct font_resource_t
{
  unsigned char *data;  // Font file data buffer (TTF/OTF bytes)
  size_t size;          // Size of font data in bytes
  stbtt_fontinfo *info; // stb_truetype font info structure
  bool valid;           // True if font loaded and initialized successfully
} font_resource_t;

// ════════════════════════════════════════════════════════════
// FONT LOADING AND MANAGEMENT
// ════════════════════════════════════════════════════════════

// Load a TrueType font from file
//
// Reads font file from disk, allocates buffers, and initializes
// stb_truetype font info for rendering.
//
// Parameters:
//   font: Font resource structure to initialize
//   path: Path to TTF/OTF font file
//
// Returns:
//   true if font loaded and initialized successfully
//   false on error (file not found, invalid format, memory allocation failure)
//
// Example:
//   font_resource_t font = {0};
//   if (font_load(&font, "resources/fonts/LiberationSans-Bold.ttf")) {
//     // Use font for rendering...
//     font_free(&font);
//   }
//
// Notes:
//   - Caller must call font_free() when done to release memory
//   - Uses logging system (LOG_DEBUG, LOG_INFO, LOG_ERROR)
//   - Sets font.valid = true on success
bool font_load(font_resource_t *font, const char *path);

// Free font resource memory
//
// Releases font data buffer and font info structure.
// Safe to call on uninitialized or already-freed fonts.
//
// Parameters:
//   font: Font resource to free
//
// Example:
//   font_resource_t font = {0};
//   font_load(&font, "font.ttf");
//   // ... use font ...
//   font_free(&font);  // Clean up
void font_free(font_resource_t *font);

// ════════════════════════════════════════════════════════════
// FONT VALIDATION
// ════════════════════════════════════════════════════════════

// Check if font resource is valid and ready for use
//
// Parameters:
//   font: Font resource to check
//
// Returns:
//   true if font was loaded successfully and is ready for rendering
//   false if font is invalid, not loaded, or was freed
static inline bool
font_is_valid(const font_resource_t *font)
{
  return font && font->valid && font->data && font->info;
}

#endif // RESOURCES_FONT_H
