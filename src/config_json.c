// JSON Configuration Parser Implementation

#include "config_json.h"

#include "cJSON.h"
#include "osd_plugin.h"
#include "rendering/blending.h"
#include "utils/logging.h"
#include "utils/resource_lookup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ════════════════════════════════════════════════════════════
// JSON GETTER HELPERS
// ════════════════════════════════════════════════════════════
//
// These functions follow a consistent pattern for safe JSON value extraction:
//   1. Fetch item from JSON object by key
//   2. Type check using cJSON_Is<Type>()
//   3. Return typed value or default if missing/wrong type
//
// This pattern provides:
//   - Type safety: Wrong types return default instead of crashing
//   - Null safety: Missing keys return default
//   - Consistent API: All getters follow same signature pattern
//   - No error logging: Defaults are valid (allows optional config fields)
//
// Usage:
//   int width = get_int(config, "width", 1920);
//   bool enabled = get_bool(config, "enabled", true);
//   const char *name = get_string(config, "name", "default");
//

/**
 * Get integer value from JSON object
 *
 * @param obj JSON object to query
 * @param key Key to look up
 * @param default_value Value to return if key missing or wrong type
 * @return Integer value or default
 */
static int
get_int(cJSON *obj, const char *key, int default_value)
{
  cJSON *item = cJSON_GetObjectItem(obj, key);
  if (item && cJSON_IsNumber(item))
    {
      return item->valueint;
    }
  return default_value;
}

/**
 * Get double value from JSON object
 *
 * @param obj JSON object to query
 * @param key Key to look up
 * @param default_value Value to return if key missing or wrong type
 * @return Double value or default
 */
static double
get_double(cJSON *obj, const char *key, double default_value)
{
  cJSON *item = cJSON_GetObjectItem(obj, key);
  if (item && cJSON_IsNumber(item))
    {
      return item->valuedouble;
    }
  return default_value;
}

/**
 * Get boolean value from JSON object
 *
 * @param obj JSON object to query
 * @param key Key to look up
 * @param default_value Value to return if key missing or wrong type
 * @return Boolean value or default
 */
static bool
get_bool(cJSON *obj, const char *key, bool default_value)
{
  cJSON *item = cJSON_GetObjectItem(obj, key);
  if (item && cJSON_IsBool(item))
    {
      return cJSON_IsTrue(item);
    }
  return default_value;
}

/**
 * Get string value from JSON object
 *
 * @param obj JSON object to query
 * @param key Key to look up
 * @param default_value Value to return if key missing or wrong type
 * @return String value (pointer into cJSON structure) or default
 *
 * Note: Returned pointer is owned by cJSON and valid until cJSON_Delete()
 */
static const char *
get_string(cJSON *obj, const char *key, const char *default_value)
{
  cJSON *item = cJSON_GetObjectItem(obj, key);
  if (item && cJSON_IsString(item))
    {
      return item->valuestring;
    }
  return default_value;
}

/**
 * Get color value from JSON object
 *
 * Parses hex color string (e.g., "#RRGGBB" or "#AARRGGBB") into
 * internal RGBA uint32_t format (0xAABBGGRR).
 *
 * @param obj JSON object to query
 * @param key Key to look up
 * @param default_value Value to return if key missing or parse fails
 * @return RGBA color value or default
 */
static uint32_t
get_color(cJSON *obj, const char *key, uint32_t default_value)
{
  const char *hex = get_string(obj, key, NULL);
  if (hex)
    {
      return parse_color(hex);
    }
  return default_value;
}

// ════════════════════════════════════════════════════════════
// JSON PARSING HELPERS
// ════════════════════════════════════════════════════════════

/**
 * Read and parse JSON file
 *
 * @param json_path Path to JSON file
 * @return Parsed cJSON root object, or NULL on error
 * @note Caller must free returned cJSON object with cJSON_Delete()
 */
static cJSON *
read_and_parse_json(const char *json_path)
{
  // Read JSON file
  FILE *fp = fopen(json_path, "r");
  if (!fp)
    {
      LOG_ERROR("Failed to open JSON file: %s", json_path);
      return NULL;
    }

  // Get file size
  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  // Check file size is valid
  if (file_size <= 0)
    {
      LOG_ERROR("Invalid file size: %ld", file_size);
      fclose(fp);
      return NULL;
    }

  // Allocate buffer and read
  size_t buffer_size = (size_t)file_size + 1;
  char *json_data    = (char *)malloc(buffer_size);
  if (!json_data)
    {
      LOG_ERROR("Failed to allocate memory");
      fclose(fp);
      return NULL;
    }

  size_t bytes_read = fread(json_data, 1, (size_t)file_size, fp);
  fclose(fp);

  // Bounds check and clamp before null termination
  size_t safe_index
    = (bytes_read < buffer_size) ? bytes_read : (buffer_size - 1);

  // NOLINTNEXTLINE(clang-analyzer-security.ArrayBound)
  // False positive: safe_index is explicitly bounded by ternary check above
  json_data[safe_index] = '\0';

  // Parse JSON
  cJSON *root = cJSON_Parse(json_data);
  free(json_data);

  if (!root)
    {
      const char *error_ptr = cJSON_GetErrorPtr();
      LOG_ERROR("JSON parse error: %s", error_ptr ? error_ptr : "unknown");
      return NULL;
    }

  return root;
}

/**
 * Parse crosshair configuration section
 */
static void
parse_crosshair_config(cJSON *root, crosshair_config_t *config)
{
  cJSON *crosshair = cJSON_GetObjectItem(root, "crosshair");
  if (!crosshair)
    return;

  config->enabled = get_bool(crosshair, "enabled", true);

  // Parse orientation
  const char *orientation = get_string(crosshair, "orientation", "vertical");
  if (strcmp(orientation, "diagonal") == 0)
    {
      config->orientation = CROSSHAIR_ORIENTATION_DIAGONAL;
    }
  else
    {
      config->orientation = CROSSHAIR_ORIENTATION_VERTICAL;
    }

  // Center dot
  cJSON *center_dot = cJSON_GetObjectItem(crosshair, "center_dot");
  if (center_dot)
    {
      config->center_dot.enabled   = get_bool(center_dot, "enabled", true);
      config->center_dot_radius    = get_int(center_dot, "radius", 3);
      config->center_dot.color     = get_color(center_dot, "color", COLOR_RED);
      config->center_dot.thickness = get_int(center_dot, "thickness", 1);
    }

  // Cross arms
  cJSON *cross = cJSON_GetObjectItem(crosshair, "cross");
  if (cross)
    {
      config->cross.enabled   = get_bool(cross, "enabled", true);
      config->cross_length    = get_int(cross, "length", 35);
      config->cross_gap       = get_int(cross, "gap", 10);
      config->cross.thickness = get_int(cross, "thickness", 4);
      config->cross.color     = get_color(cross, "color", COLOR_RED);
    }

  // Circle
  cJSON *circle = cJSON_GetObjectItem(crosshair, "circle");
  if (circle)
    {
      config->circle.enabled   = get_bool(circle, "enabled", true);
      config->circle_radius    = get_int(circle, "radius", 15);
      config->circle.thickness = get_int(circle, "thickness", 2);
      config->circle.color     = get_color(circle, "color", COLOR_RED);
    }
}

/**
 * Parse timestamp configuration section
 */
static void
parse_timestamp_config(cJSON *root, timestamp_config_t *config)
{
  cJSON *timestamp = cJSON_GetObjectItem(root, "timestamp");
  if (!timestamp)
    return;

  config->enabled   = get_bool(timestamp, "enabled", true);
  config->pos_x     = get_int(timestamp, "position_x", 10);
  config->pos_y     = get_int(timestamp, "position_y", 10);
  config->color     = get_color(timestamp, "color", COLOR_CYAN);
  config->font_size = get_int(timestamp, "font_size", 14);

  // Parse font name and resolve to path using registry
  const char *font_name = get_string(timestamp, "font", "liberation_sans_bold");
  const char *font_path = get_font_path(font_name);
  if (font_path)
    {
      strncpy(config->font_path, font_path, sizeof(config->font_path) - 1);
    }
}

/**
 * Parse speed indicators configuration section
 */
static void
parse_speed_indicators_config(cJSON *root, speed_config_t *config)
{
  cJSON *speed_indicators = cJSON_GetObjectItem(root, "speed_indicators");
  if (!speed_indicators)
    return;

  config->enabled   = get_bool(speed_indicators, "enabled", true);
  config->color     = get_color(speed_indicators, "color", COLOR_GREEN);
  config->font_size = get_int(speed_indicators, "font_size", 14);
  config->threshold = (float)get_double(speed_indicators, "threshold", 0.05);
  config->max_speed_azimuth
    = (float)get_double(speed_indicators, "max_speed_azimuth", 35.0);
  config->max_speed_elevation
    = (float)get_double(speed_indicators, "max_speed_elevation", 35.0);

  // Parse font name and resolve to path using registry
  const char *font_name
    = get_string(speed_indicators, "font", "liberation_sans_bold");
  const char *font_path = get_font_path(font_name);
  if (font_path)
    {
      strncpy(config->font_path, font_path, sizeof(config->font_path) - 1);
    }
}

/**
 * Parse variant info configuration section
 */
static void
parse_variant_info_config(cJSON *root, variant_info_config_t *config)
{
  cJSON *variant_info = cJSON_GetObjectItem(root, "variant_info");
  if (!variant_info)
    return;

  config->enabled   = get_bool(variant_info, "enabled", true);
  config->pos_x     = get_int(variant_info, "position_x", 10);
  config->pos_y     = get_int(variant_info, "position_y", 50);
  config->color     = get_color(variant_info, "color", COLOR_YELLOW);
  config->font_size = get_int(variant_info, "font_size", 14);

  // Parse font name and resolve to path using registry
  const char *font_name
    = get_string(variant_info, "font", "liberation_sans_bold");
  const char *font_path = get_font_path(font_name);
  if (font_path)
    {
      strncpy(config->font_path, font_path, sizeof(config->font_path) - 1);
    }
}

/**
 * Parse navball configuration section
 */
static void
parse_navball_config(cJSON *root, navball_config_t *config)
{
  cJSON *navball = cJSON_GetObjectItem(root, "navball");
  if (!navball)
    return;

  config->enabled    = get_bool(navball, "enabled", true);
  config->position_x = get_int(navball, "position_x", 810);
  config->position_y = get_int(navball, "position_y", 730);
  config->size       = get_int(navball, "size", 300);

  // Parse skin name using registry lookup
  const char *skin_name = get_string(navball, "skin", "5thHorseman_v2");
  config->skin          = get_navball_skin_by_name(skin_name);

  config->show_level_marker = get_bool(navball, "show_level_marker", false);

  // Parse center indicator configuration
  cJSON *center_indicator = cJSON_GetObjectItem(navball, "center_indicator");
  if (center_indicator)
    {
      config->show_center_indicator
        = get_bool(center_indicator, "enabled", false);
      config->center_indicator_scale
        = (float)get_double(center_indicator, "scale", 0.2);

      // Get indicator SVG path from resource lookup
      const char *indicator_name
        = get_string(center_indicator, "indicator", "circle");
      const char *indicator_path = get_indicator_path(indicator_name);
      if (indicator_path)
        {
          strncpy(config->center_indicator_svg_path, indicator_path,
                  sizeof(config->center_indicator_svg_path) - 1);
        }
    }
}

/**
 * Parse celestial indicators configuration
 *
 * Extracts celestial indicators (sun/moon) configuration from JSON.
 * Defaults to enabled with -5° visibility threshold.
 *
 * @param root Root JSON object
 * @param config Celestial indicators configuration to populate
 */
static void
parse_celestial_indicators_config(cJSON *root,
                                  celestial_indicators_config_t *config)
{
  cJSON *celestial = cJSON_GetObjectItem(root, "celestial_indicators");
  if (!celestial)
    {
      // Default: disabled if not present
      config->enabled = false;
      return;
    }

  config->enabled         = get_bool(celestial, "enabled", true);
  config->show_sun        = get_bool(celestial, "show_sun", true);
  config->show_moon       = get_bool(celestial, "show_moon", true);
  config->indicator_scale = (float)get_double(celestial, "scale", 1.0);
  config->visibility_threshold
    = (float)get_double(celestial, "visibility_threshold", -5.0);

  // Parse SVG paths
  const char *sun_front_path = get_string(
    celestial, "sun_front_svg", "resources/navball_indicators/sun_front.svg");
  strncpy(config->sun_front_svg_path, sun_front_path,
          sizeof(config->sun_front_svg_path) - 1);

  const char *sun_back_path = get_string(
    celestial, "sun_back_svg", "resources/navball_indicators/sun_back.svg");
  strncpy(config->sun_back_svg_path, sun_back_path,
          sizeof(config->sun_back_svg_path) - 1);

  const char *moon_front_path = get_string(
    celestial, "moon_front_svg", "resources/navball_indicators/moon_front.svg");
  strncpy(config->moon_front_svg_path, moon_front_path,
          sizeof(config->moon_front_svg_path) - 1);

  const char *moon_back_path = get_string(
    celestial, "moon_back_svg", "resources/navball_indicators/moon_back.svg");
  strncpy(config->moon_back_svg_path, moon_back_path,
          sizeof(config->moon_back_svg_path) - 1);
}

/**
 * Parse sharpness heatmap configuration
 */
static void
parse_sharpness_heatmap_config(cJSON *root, sharpness_heatmap_config_t *config)
{
  cJSON *heatmap = cJSON_GetObjectItem(root, "sharpness_heatmap");
  if (!heatmap)
    {
      config->enabled = false;
      return;
    }

  config->enabled         = get_bool(heatmap, "enabled", true);
  config->pos_x           = get_int(heatmap, "position_x", 1810);
  config->pos_y           = get_int(heatmap, "position_y", 970);
  config->cell_size       = get_int(heatmap, "cell_size", 12);
  config->show_label      = get_bool(heatmap, "show_label", true);
  config->label_font_size = get_int(heatmap, "label_font_size", 16);
}

/**
 * Parse detections overlay configuration
 */
static void
parse_detections_config(cJSON *root, detections_config_t *config)
{
  cJSON *detections = cJSON_GetObjectItem(root, "detections");
  if (!detections)
    {
      config->enabled = false;
      return;
    }

  config->enabled         = get_bool(detections, "enabled", true);
  config->color           = get_color(detections, "color", 0xFF00FF00);
  config->box_thickness   = (float)get_double(detections, "box_thickness", 2.0);
  config->per_class_color = get_bool(detections, "per_class_color", true);
  config->label_font_size = get_int(detections, "label_font_size", 16);
  config->min_confidence
    = (float)get_double(detections, "min_confidence", 0.25);
}

/**
 * Parse ROI overlay configuration
 */
static void
parse_roi_config(cJSON *root, roi_config_t *config)
{
  cJSON *roi = cJSON_GetObjectItem(root, "roi");
  if (!roi)
    {
      config->enabled         = true;
      config->box_thickness   = 2.0f;
      config->label_font_size = 14;
      config->color_focus     = 0xFF00FF00;
      config->color_track     = 0xFF00FFFF;
      config->color_zoom      = 0xFFFF00FF;
      config->color_fx        = 0xFFFFFF00;
      return;
    }

  config->enabled         = get_bool(roi, "enabled", true);
  config->box_thickness   = (float)get_double(roi, "box_thickness", 2.0);
  config->label_font_size = get_int(roi, "label_font_size", 14);
  config->color_focus     = get_color(roi, "color_focus", 0xFF00FF00);
  config->color_track     = get_color(roi, "color_track", 0xFF00FFFF);
  config->color_zoom      = get_color(roi, "color_zoom", 0xFFFF00FF);
  config->color_fx        = get_color(roi, "color_fx", 0xFFFFFF00);
}

/**
 * Parse autofocus debug panel configuration
 */
static void
parse_autofocus_debug_config(cJSON *root, autofocus_debug_config_t *config)
{
  cJSON *af_debug = cJSON_GetObjectItem(root, "autofocus_debug");
  if (!af_debug)
    {
      config->enabled = false;
      return;
    }

  config->enabled           = get_bool(af_debug, "enabled", false);
  config->pos_x             = get_int(af_debug, "position_x", 690);
  config->pos_y             = get_int(af_debug, "position_y", 800);
  config->bar_height        = get_int(af_debug, "bar_height", 80);
  config->heatmap_cell_size = get_int(af_debug, "heatmap_cell_size", 12);
  config->chart_width       = get_int(af_debug, "chart_width", 180);
}

/**
 * Parse SAM tracking overlay configuration
 */
static void
parse_sam_mask_config(cJSON *root, sam_mask_config_t *config)
{
  cJSON *sam_mask = cJSON_GetObjectItem(root, "sam_mask");
  if (!sam_mask)
    {
      config->enabled = false;
      return;
    }

  config->enabled         = get_bool(sam_mask, "enabled", true);
  config->color           = get_color(sam_mask, "color", 0xFF00FF00);
  config->box_thickness   = (float)get_double(sam_mask, "box_thickness", 2.0);
  config->per_state_color = get_bool(sam_mask, "per_state_color", true);
  config->label_font_size = get_int(sam_mask, "label_font_size", 14);
  config->centroid_radius = get_int(sam_mask, "centroid_radius", 8);
  config->mask_enabled    = get_bool(sam_mask, "mask_enabled", true);
  config->mask_alpha      = (uint8_t)get_int(sam_mask, "mask_alpha", 128);
}

// ════════════════════════════════════════════════════════════
// JSON PARSING IMPLEMENTATION
// ════════════════════════════════════════════════════════════

bool
config_parse_json(osd_config_t *config, const char *json_path)
{
  if (!config || !json_path)
    {
      LOG_ERROR("Invalid arguments");
      return false;
    }

  // Read and parse JSON (delegates file I/O to helper)
  cJSON *root = read_and_parse_json(json_path);
  if (!root)
    return false;

  LOG_INFO("Parsing JSON config: %s", json_path);

  // Parse each configuration section (delegates to focused helpers)
  // Each widget has its own font setting parsed in its section
  parse_crosshair_config(root, &config->crosshair);
  parse_timestamp_config(root, &config->timestamp);
  parse_speed_indicators_config(root, &config->speed_indicators);
  parse_variant_info_config(root, &config->variant_info);
  parse_navball_config(root, &config->navball);
  parse_celestial_indicators_config(root, &config->celestial_indicators);
  parse_sharpness_heatmap_config(root, &config->sharpness_heatmap);
  parse_detections_config(root, &config->detections);
  parse_roi_config(root, &config->roi);
  parse_autofocus_debug_config(root, &config->autofocus_debug);
  parse_sam_mask_config(root, &config->sam_mask);

  // Clean up
  cJSON_Delete(root);

  LOG_INFO("JSON config parsed successfully");
  return true;
}
