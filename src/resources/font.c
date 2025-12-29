#include "resources/font.h"

#include "utils/logging.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// stb_truetype implementation (single-header library)
// Note: stb_truetype uses math functions without including math.h
// WASI SDK strict mode would error, so we disable that warning
#ifndef isnan
#define isnan(x) __builtin_isnan(x)
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-function-declaration"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#pragma clang diagnostic pop

// ════════════════════════════════════════════════════════════
// FONT LOADING IMPLEMENTATION
// ════════════════════════════════════════════════════════════

bool
font_load(font_resource_t *font, const char *path)
{
  if (!font || !path)
    {
      LOG_ERROR("Invalid arguments to font_load()");
      return false;
    }

  // Initialize font structure
  memset(font, 0, sizeof(font_resource_t));

  LOG_DEBUG("Loading font from file: %s", path);

  // Open font file via WASI
  FILE *fp = fopen(path, "rb");
  if (!fp)
    {
      LOG_ERROR("Failed to open font file: %s (errno=%d)", path, errno);
      return false;
    }

  LOG_DEBUG("Font file opened successfully");

  // Get file size
  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (file_size <= 0)
    {
      LOG_ERROR("Invalid font file size: %ld", file_size);
      fclose(fp);
      return false;
    }

  // Allocate buffer for font data
  font->data = (unsigned char *)malloc(file_size);
  if (!font->data)
    {
      LOG_ERROR("Failed to allocate font buffer (%ld bytes)", file_size);
      fclose(fp);
      return false;
    }

  // Read font data
  size_t bytes_read = fread(font->data, 1, file_size, fp);
  fclose(fp);

  if (bytes_read != (size_t)file_size)
    {
      LOG_ERROR("Failed to read font data (read %zu, expected %ld)", bytes_read,
                file_size);
      free(font->data);
      font->data = NULL;
      return false;
    }

  font->size = file_size;
  LOG_INFO("Font file loaded: %ld bytes", file_size);

  // Allocate and initialize stbtt_fontinfo
  font->info = (stbtt_fontinfo *)malloc(sizeof(stbtt_fontinfo));
  if (!font->info)
    {
      LOG_ERROR("Failed to allocate font_info");
      free(font->data);
      memset(font, 0, sizeof(*font)); // Clear all fields for safety
      return false;
    }

  // Initialize font
  if (!stbtt_InitFont(font->info, font->data, 0))
    {
      LOG_ERROR("Failed to initialize font");
      free(font->info);
      free(font->data);
      memset(font, 0, sizeof(*font)); // Clear all fields for safety
      return false;
    }

  font->valid = true;
  LOG_INFO("Font initialized successfully");
  return true;
}

// ════════════════════════════════════════════════════════════
// FONT CLEANUP IMPLEMENTATION
// ════════════════════════════════════════════════════════════

void
font_free(font_resource_t *font)
{
  if (!font)
    {
      return;
    }

  if (font->info)
    {
      free(font->info);
      font->info = NULL;
    }

  if (font->data)
    {
      free(font->data);
      font->data = NULL;
    }

  font->size  = 0;
  font->valid = false;
}
