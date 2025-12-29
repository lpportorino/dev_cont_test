/**
 * @file main.c
 * @brief Video test harness main driver
 *
 * Generates test videos for all 4 OSD variants by:
 * 1. Validating JSON configuration (JSON Schema Draft-07)
 * 2. Loading WASM module
 * 3. Creating GStreamer pipeline (noise/static background + OSD overlay)
 * 4. Generating synthetic protobuf states
 * 5. Rendering frames and encoding to MP4
 */

#include "config_validator.h"
#include "gst_pipeline.h"
#include "synthetic_state.h"
#include "wasm_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Variant configuration
typedef struct
{
  const char *name;
  const char *wasm_path;
  const char *config_path;
  const char *output_path;
  uint32_t width;
  uint32_t height;
} variant_config_t;

static const variant_config_t VARIANTS[] = {
  { .name = "live_day",
    .wasm_path = "../../build/live_day.wasm",
    .config_path = "../../resources/live_day.json",
    .output_path = "../output/live_day.mp4",
    .width = 1920,
    .height = 1080 },
  { .name = "live_thermal",
    .wasm_path = "../../build/live_thermal.wasm",
    .config_path = "../../resources/live_thermal.json",
    .output_path = "../output/live_thermal.mp4",
    .width = 900,
    .height = 720 },
  { .name = "recording_day",
    .wasm_path = "../../build/recording_day.wasm",
    .config_path = "../../resources/recording_day.json",
    .output_path = "../output/recording_day.mp4",
    .width = 1920,
    .height = 1080 },
  { .name = "recording_thermal",
    .wasm_path = "../../build/recording_thermal.wasm",
    .config_path = "../../resources/recording_thermal.json",
    .output_path = "../output/recording_thermal.mp4",
    .width = 900,
    .height = 720 },
};

#define NUM_VARIANTS (sizeof (VARIANTS) / sizeof (VARIANTS[0]))

// Video parameters
#define VIDEO_DURATION_SECONDS 10.0f
#define VIDEO_FPS 30

static void
ensure_output_directory (void)
{
  struct stat st = { 0 };
  if (stat ("../output", &st) == -1)
    {
      printf ("Creating output directory: ../output\n");
      mkdir ("../output", 0755);
    }
}

static bool
generate_video_for_variant (const variant_config_t *variant)
{
  printf ("\n");
  printf ("========================================\n");
  printf ("  Generating: %s\n", variant->name);
  printf ("========================================\n");
  printf ("\n");

  bool success = false;
  osd_wasm_module_t *wasm = NULL;
  gst_pipeline_t *pipeline = NULL;
  synthetic_state_t *state_gen = NULL;

  // 0. Validate JSON configuration
  const char *schema_path = "../../resources/schemas/osd_config.schema.json";
  printf ("[MAIN] Validating config: %s\n", variant->config_path);
  if (!validate_config (variant->config_path, schema_path))
    {
      fprintf (stderr, "[MAIN] Configuration validation FAILED\n");
      fprintf (stderr, "[MAIN] Fix the errors above and try again\n");
      goto cleanup;
    }
  printf ("[MAIN] Configuration validated successfully\n");
  printf ("\n");

  // 1. Load WASM module
  printf ("[MAIN] Loading WASM module: %s\n", variant->wasm_path);
  wasm = wasm_module_load (variant->wasm_path, variant->width, variant->height);
  if (!wasm)
    {
      fprintf (stderr, "[MAIN] Failed to load WASM module\n");
      goto cleanup;
    }

  if (wasm_module_init (wasm) != 0)
    {
      fprintf (stderr, "[MAIN] Failed to initialize WASM module\n");
      goto cleanup;
    }

  // 2. Create GStreamer pipeline
  printf ("[MAIN] Creating GStreamer pipeline\n");
  uint32_t num_frames = (uint32_t)(VIDEO_DURATION_SECONDS * VIDEO_FPS);
  pipeline = gst_pipeline_create (variant->width, variant->height, VIDEO_FPS,
                                   num_frames, variant->output_path);
  if (!pipeline)
    {
      fprintf (stderr, "[MAIN] Failed to create pipeline\n");
      goto cleanup;
    }

  if (!gst_pipeline_start (pipeline))
    {
      fprintf (stderr, "[MAIN] Failed to start pipeline\n");
      goto cleanup;
    }

  // 3. Create synthetic state generator
  printf ("[MAIN] Creating synthetic state generator\n");
  state_gen = synthetic_state_create (ANIM_CIRCLE, VIDEO_DURATION_SECONDS,
                                       VIDEO_FPS);
  if (!state_gen)
    {
      fprintf (stderr, "[MAIN] Failed to create state generator\n");
      goto cleanup;
    }

  // 4. Rendering loop
  printf ("[MAIN] Rendering %u frames...\n",
          (uint32_t)(VIDEO_DURATION_SECONDS * VIDEO_FPS));

  uint64_t timestamp = 0;
  uint64_t frame_duration_ns = 1000000000 / VIDEO_FPS;
  uint32_t frame_count = 0;

  while (synthetic_state_next_frame (state_gen))
    {
      // Get encoded state
      size_t state_size;
      const uint8_t *state_data
        = synthetic_state_get_encoded (state_gen, &state_size);
      if (!state_data)
        {
          fprintf (stderr, "[MAIN] Failed to encode state\n");
          goto cleanup;
        }

      // Update WASM state
      if (wasm_module_update_state (wasm, state_data, state_size) != 0)
        {
          fprintf (stderr, "[MAIN] Failed to update WASM state\n");
          goto cleanup;
        }

      // Get framebuffer pointer BEFORE rendering
      uint8_t *framebuffer = wasm_module_get_framebuffer (wasm);
      if (!framebuffer)
        {
          fprintf (stderr, "[MAIN] Failed to get framebuffer\n");
          goto cleanup;
        }

      // Render OSD (will be composited with checkerboard background by GStreamer)
      int render_ret = wasm_module_render (wasm);
      if (render_ret < 0)
        {
          fprintf (stderr, "[MAIN] Failed to render frame\n");
          goto cleanup;
        }

      // Push frame to GStreamer
      if (!gst_pipeline_push_frame (pipeline, framebuffer, timestamp))
        {
          fprintf (stderr, "[MAIN] Failed to push frame\n");
          goto cleanup;
        }

      timestamp += frame_duration_ns;
      frame_count++;

      // Progress indicator
      if (frame_count % 30 == 0)
        {
          float progress = (float)frame_count
                           / (VIDEO_DURATION_SECONDS * VIDEO_FPS) * 100.0f;
          printf ("[MAIN] Progress: %.1f%% (%u frames)\n", progress,
                  frame_count);
        }
    }

  printf ("[MAIN] Rendered %u frames\n", frame_count);

  // 5. Finish pipeline
  printf ("[MAIN] Finalizing video...\n");
  gst_pipeline_finish (pipeline);

  printf ("[MAIN] ✅ Video generated: %s\n", variant->output_path);
  success = true;

cleanup:
  if (state_gen)
    synthetic_state_destroy (state_gen);
  if (pipeline)
    gst_pipeline_destroy (pipeline);
  if (wasm)
    wasm_module_destroy (wasm);

  return success;
}

int
main (int argc, char *argv[])
{
  printf ("========================================\n");
  printf ("  OSD Video Test Harness\n");
  printf ("========================================\n");
  printf ("\n");
  printf ("Configuration:\n");
  printf ("  Duration: %.1f seconds\n", VIDEO_DURATION_SECONDS);
  printf ("  FPS: %u\n", VIDEO_FPS);
  printf ("  Animation: Circular rotation\n");
  printf ("  Variants: %zu\n", NUM_VARIANTS);
  printf ("\n");

  // Ensure output directory exists
  ensure_output_directory ();

  // Process specific variant if specified via command line
  if (argc > 1)
    {
      const char *variant_name = argv[1];
      printf ("Generating single variant: %s\n\n", variant_name);

      for (size_t i = 0; i < NUM_VARIANTS; i++)
        {
          if (strcmp (VARIANTS[i].name, variant_name) == 0)
            {
              bool success = generate_video_for_variant (&VARIANTS[i]);
              return success ? 0 : 1;
            }
        }

      fprintf (stderr, "Unknown variant: %s\n", variant_name);
      fprintf (stderr, "Available variants: ");
      for (size_t i = 0; i < NUM_VARIANTS; i++)
        {
          fprintf (stderr, "%s%s", VARIANTS[i].name,
                   (i < NUM_VARIANTS - 1) ? ", " : "\n");
        }
      return 1;
    }

  // Generate all variants
  uint32_t success_count = 0;
  for (size_t i = 0; i < NUM_VARIANTS; i++)
    {
      if (generate_video_for_variant (&VARIANTS[i]))
        {
          success_count++;
        }
      else
        {
          fprintf (stderr, "[MAIN] ❌ Failed to generate %s\n",
                   VARIANTS[i].name);
        }
    }

  printf ("\n");
  printf ("========================================\n");
  printf ("  Summary\n");
  printf ("========================================\n");
  printf ("  Successfully generated: %u / %zu videos\n", success_count,
          NUM_VARIANTS);
  printf ("\n");

  if (success_count == NUM_VARIANTS)
    {
      printf ("✅ All videos generated successfully!\n");
      printf ("\n");
      printf ("Output files:\n");
      for (size_t i = 0; i < NUM_VARIANTS; i++)
        {
          printf ("  %s\n", VARIANTS[i].output_path);
        }
      printf ("\n");
      return 0;
    }
  else
    {
      printf ("⚠️  Some videos failed to generate\n");
      return 1;
    }
}
