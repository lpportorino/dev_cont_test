#ifndef RESOURCE_LOOKUP_H
#define RESOURCE_LOOKUP_H

// Resource lookup utilities
// Maps simple names to full resource paths for fonts and indicators

#include "../osd_plugin.h"

#include <stdbool.h>

// Get full path for a font name
// Returns path string on success, NULL if font not found
const char *get_font_path(const char *font_name);

// Get navball skin enum by name
// Returns navball_skin_t enum on success, NAVBALL_SKIN_STOCK on failure/default
navball_skin_t get_navball_skin_by_name(const char *skin_name);

// Get full path for a navball indicator name
// Returns path string on success, NULL if indicator not found
const char *get_indicator_path(const char *indicator_name);

// List all available fonts (for validation/debugging)
void list_available_fonts(void);

// List all available navball skins (for validation/debugging)
void list_available_skins(void);

// List all available indicators (for validation/debugging)
void list_available_indicators(void);

#endif // RESOURCE_LOOKUP_H
