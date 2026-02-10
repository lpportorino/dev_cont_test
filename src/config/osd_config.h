#ifndef OSD_CONFIG_H
#define OSD_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

// ════════════════════════════════════════════════════════════
// CONFIGURATION STRUCTURES
// ════════════════════════════════════════════════════════════
//
// This file contains all OSD configuration structures that are
// loaded from JSON configuration files at runtime. These structures
// define the appearance and behavior of all OSD widgets.
//
// ════════════════════════════════════════════════════════════

// Crosshair orientation
typedef enum
{
  CROSSHAIR_ORIENTATION_VERTICAL, // Standard + shape
  CROSSHAIR_ORIENTATION_DIAGONAL  // X shape (45 degrees)
} crosshair_orientation_t;

// Individual crosshair element configuration
typedef struct
{
  bool enabled;
  uint32_t color;  // ARGB format (0xAARRGGBB)
  float thickness; // Pixels
} crosshair_element_t;

// Crosshair configuration (loaded from config.xml)
typedef struct
{
  // Overall enable
  bool enabled;

  // Orientation
  crosshair_orientation_t orientation;

  // Center dot
  crosshair_element_t center_dot;
  float center_dot_radius;

  // Cross arms (4 lines)
  crosshair_element_t cross;
  float cross_length; // Length of each arm
  float cross_gap;    // Gap from center

  // Circle
  crosshair_element_t circle;
  float circle_radius;

  // SVG sources (optional - if empty, use primitive rendering)
  char cross_svg_path[256];
  char circle_svg_path[256];
} crosshair_config_t;

// Timestamp configuration
typedef struct
{
  bool enabled;
  uint32_t color;
  int font_size;
  int pos_x;
  int pos_y;
  char font_path[256];
} timestamp_config_t;

// Speed indicators configuration
typedef struct
{
  bool enabled;
  uint32_t color;
  int font_size;
  float threshold;           // Min normalized speed (0.0-1.0) to show indicator
  float max_speed_azimuth;   // Max speed in degrees/s for azimuth
  float max_speed_elevation; // Max speed in degrees/s for elevation
  char font_path[256];
} speed_config_t;

// Variant info widget configuration
typedef struct
{
  bool enabled;
  int pos_x;
  int pos_y;
  uint32_t color;
  int font_size;
  char font_path[256];
} variant_info_config_t;

// Nav ball skin types
typedef enum
{
  NAVBALL_SKIN_STOCK = 0,          // stock.png - Default KSP nav ball
  NAVBALL_SKIN_STOCK_IVA,          // stock-iva.png - IVA variant
  NAVBALL_SKIN_5TH_HORSEMAN_V2,    // 5thHorseman_v2-navball.png
  NAVBALL_SKIN_5TH_HORSEMAN_BLACK, // 5thHorseman-navball_blackgrey_DIF.png
  NAVBALL_SKIN_5TH_HORSEMAN_BROWN, // 5thHorseman-navball_brownblue_DIF.png
  NAVBALL_SKIN_JAFO,               // JAFO.png
  NAVBALL_SKIN_KBOB_V2,            // kBob_v2.2.png
  NAVBALL_SKIN_ORDINARY_KERMAN,    // OrdinaryKerman.png
  NAVBALL_SKIN_TREKKY,             // Trekky0623_DIF.png
  NAVBALL_SKIN_APOLLO,             // tooRelic_Apollo.png
  NAVBALL_SKIN_WHITE_OWL,          // White_Owl.png
  NAVBALL_SKIN_ZASNOLD,            // Zasnold_DIF.png
  NAVBALL_SKIN_FALCONB,            // FalconB.png
  NAVBALL_SKIN_COUNT               // Total number of skins
} navball_skin_t;

// Nav ball configuration
typedef struct
{
  bool enabled;
  int position_x;
  int position_y;
  int size;
  navball_skin_t skin;
  bool show_level_marker;

  // Center indicator overlay
  bool show_center_indicator;
  float center_indicator_scale; // Scale factor (1.0 = 100% of navball size)
  char center_indicator_svg_path[256];
} navball_config_t;

// Celestial indicators configuration (sun and moon on navball)
typedef struct
{
  bool enabled;               // Master enable for all celestial indicators
  bool show_sun;              // Show sun indicator
  bool show_moon;             // Show moon indicator
  float indicator_scale;      // Scale factor (1.0 = 100% of default size)
  float visibility_threshold; // Min altitude (degrees) to show indicator (e.g.,
                              // -5.0)
  char sun_front_svg_path[256];  // Sun visible (altitude > 0)
  char sun_back_svg_path[256];   // Sun behind horizon (altitude < 0)
  char moon_front_svg_path[256]; // Moon visible
  char moon_back_svg_path[256];  // Moon behind horizon
} celestial_indicators_config_t;

// Sharpness heatmap widget configuration
typedef struct
{
  bool enabled;
  int pos_x;
  int pos_y;
  int cell_size;   // Pixels per cell
  bool show_label; // "Sharp: 0.XXX" above grid
  int label_font_size;
} sharpness_heatmap_config_t;

// YOLO detections overlay configuration
typedef struct
{
  bool enabled;
  uint32_t color;       // Default box color (internal ARGB)
  float box_thickness;  // Outline px
  bool per_class_color; // Cycle 8-color palette by class_id
  int label_font_size;
  float min_confidence; // Display threshold [0.0-1.0]
} detections_config_t;

// Full OSD configuration
typedef struct
{
  crosshair_config_t crosshair;
  timestamp_config_t timestamp;
  speed_config_t speed_indicators;
  variant_info_config_t variant_info;
  navball_config_t navball;
  celestial_indicators_config_t celestial_indicators;
  sharpness_heatmap_config_t sharpness_heatmap;
  detections_config_t detections;
} osd_config_t;

#endif // OSD_CONFIG_H
