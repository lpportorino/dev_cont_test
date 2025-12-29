#include "resource_lookup.h"

#include "logging.h"

#include <stdio.h>
#include <string.h>

// ════════════════════════════════════════════════════════════
// FONT REGISTRY
// ════════════════════════════════════════════════════════════

typedef struct
{
  const char *name;
  const char *path;
} resource_entry_t;

// Explicit size declaration to help static analyzer understand sentinel pattern
static const resource_entry_t font_registry[5] = {
  { "liberation_sans_bold", "resources/fonts/LiberationSans-Bold.ttf" },
  { "b612_mono_bold", "resources/fonts/B612Mono-Bold.ttf" },
  { "share_tech_mono", "resources/fonts/ShareTechMono-Regular.ttf" },
  { "orbitron_bold", "resources/fonts/Orbitron-Bold.ttf" },
  { NULL, NULL } // Sentinel
};

const char *
get_font_path(const char *font_name)
{
  if (!font_name || font_name[0] == '\0')
    {
      return NULL;
    }

  // NOLINTNEXTLINE(clang-analyzer-security.ArrayBound)
  // False positive: Sentinel pattern is valid, loop terminates at NULL entry
  for (int i = 0; font_registry[i].name != NULL; i++)
    {
      if (strcmp(font_registry[i].name, font_name) == 0)
        {
          return font_registry[i].path;
        }
    }

  LOG_WARN("Font not found: %s", font_name);
  return NULL;
}

void
list_available_fonts(void)
{
  printf("Available fonts:\\n");
  // NOLINTNEXTLINE(clang-analyzer-security.ArrayBound)
  // False positive: Sentinel pattern is valid, loop terminates at NULL entry
  for (int i = 0; font_registry[i].name != NULL; i++)
    {
      printf("  - %s → %s\\n", font_registry[i].name, font_registry[i].path);
    }
}

// ════════════════════════════════════════════════════════════
// NAVBALL SKIN REGISTRY
// ════════════════════════════════════════════════════════════

#include "../osd_plugin.h"

typedef struct
{
  const char *name;
  navball_skin_t skin;
} skin_entry_t;

static const skin_entry_t skin_registry[] = {
  { "stock", NAVBALL_SKIN_STOCK },
  { "stock_iva", NAVBALL_SKIN_STOCK_IVA },
  { "5thHorseman_v2", NAVBALL_SKIN_5TH_HORSEMAN_V2 },
  { "5thHorseman_black", NAVBALL_SKIN_5TH_HORSEMAN_BLACK },
  { "5thHorseman_brown", NAVBALL_SKIN_5TH_HORSEMAN_BROWN },
  { "jafo", NAVBALL_SKIN_JAFO },
  { "kbob_v2", NAVBALL_SKIN_KBOB_V2 },
  { "ordinary_kerman", NAVBALL_SKIN_ORDINARY_KERMAN },
  { "trekky", NAVBALL_SKIN_TREKKY },
  { "apollo", NAVBALL_SKIN_APOLLO },
  { "white_owl", NAVBALL_SKIN_WHITE_OWL },
  { "zasnold", NAVBALL_SKIN_ZASNOLD },
  { "falconb", NAVBALL_SKIN_FALCONB },
  { NULL, 0 } // Sentinel
};

navball_skin_t
get_navball_skin_by_name(const char *skin_name)
{
  if (!skin_name || skin_name[0] == '\0')
    {
      return NAVBALL_SKIN_STOCK; // Default
    }

  for (int i = 0; skin_registry[i].name != NULL; i++)
    {
      if (strcmp(skin_registry[i].name, skin_name) == 0)
        {
          return skin_registry[i].skin;
        }
    }

  LOG_WARN("Navball skin not found: %s (using stock)", skin_name);
  return NAVBALL_SKIN_STOCK; // Default fallback
}

void
list_available_skins(void)
{
  printf("Available navball skins:\n");
  for (int i = 0; skin_registry[i].name != NULL; i++)
    {
      printf("  - %s\n", skin_registry[i].name);
    }
}

// ════════════════════════════════════════════════════════════
// NAVBALL INDICATOR REGISTRY
// ════════════════════════════════════════════════════════════

static const resource_entry_t indicator_registry[] = {
  { "circle", "resources/navball_indicators/circle_indicator.svg" },
  { "rectangle", "resources/navball_indicators/rectangle_indicator.svg" },
  { "crosshair", "resources/navball_indicators/crosshair_indicator.svg" },
  { NULL, NULL } // Sentinel
};

const char *
get_indicator_path(const char *indicator_name)
{
  if (!indicator_name || indicator_name[0] == '\0')
    {
      return NULL;
    }

  for (int i = 0; indicator_registry[i].name != NULL; i++)
    {
      if (strcmp(indicator_registry[i].name, indicator_name) == 0)
        {
          return indicator_registry[i].path;
        }
    }

  LOG_WARN("Indicator not found: %s", indicator_name);
  return NULL;
}

void
list_available_indicators(void)
{
  printf("Available navball indicators:\\n");
  for (int i = 0; indicator_registry[i].name != NULL; i++)
    {
      printf("  - %s → %s\\n", indicator_registry[i].name,
             indicator_registry[i].path);
    }
}
