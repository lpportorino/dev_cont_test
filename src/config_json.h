// JSON Configuration Parser
// Parses OSD configuration from JSON files using cJSON library

#ifndef CONFIG_JSON_H
#define CONFIG_JSON_H

#include "osd_plugin.h"

#include <stdbool.h>

// Parse JSON configuration file and populate osd_config_t structure
//
// Loads JSON from the given path and extracts all configuration values
// (colors, positions, sizes, enable flags) for all widgets.
//
// Parameters:
//   config:    Pointer to osd_config_t structure to populate
//   json_path: Path to JSON configuration file (accessible via WASI)
//
// Returns:
//   true on success, false on error (file not found, parse error, etc.)
//
// Usage:
//   osd_config_t config;
//   if (!config_parse_json(&config, "resources/config.json")) {
//       fprintf(stderr, "Failed to load config\n");
//       return -1;
//   }
//
// Notes:
//   - Colors in JSON use web standard format (#AARRGGBB)
//   - Parser automatically converts to internal RGBA format
//   - Missing elements use sensible defaults
//   - Invalid values are logged and defaults are used
bool config_parse_json(osd_config_t *config, const char *json_path);

#endif // CONFIG_JSON_H
