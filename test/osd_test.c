// PNG Test Harness for OSD WASM Module
// Uses Wasmtime C API to load and execute osd.wasm, then saves framebuffer as PNG

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// stb_image_write for PNG output
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vendor/stb_image_write.h"

// Wasmtime C API
#include <wasmtime.h>

#define OUTPUT_PNG "snapshot/osd_render.png"

typedef struct {
  const char *name;
  uint32_t width;
  uint32_t height;
} variant_config_t;

static const variant_config_t VARIANTS[] = {
  {"live_day", 1920, 1080},
  {"live_thermal", 900, 720},
  {"recording_day", 1920, 1080},
  {"recording_thermal", 900, 720}
};
static const size_t NUM_VARIANTS = sizeof(VARIANTS) / sizeof(VARIANTS[0]);

static const variant_config_t *
detect_variant(const char *wasm_path)
{
  // Extract variant name from path (e.g., "build/live_day.wasm" → "live_day")
  for (size_t i = 0; i < NUM_VARIANTS; i++)
    {
      if (strstr(wasm_path, VARIANTS[i].name) != NULL)
        {
          return &VARIANTS[i];
        }
    }
  return NULL;
}

static void
exit_with_error(const char *message, wasmtime_error_t *error, wasm_trap_t *trap)
{
  fprintf(stderr, "error: %s\n", message);
  wasm_byte_vec_t error_message;
  if (error != NULL)
    {
      wasmtime_error_message(error, &error_message);
      fprintf(stderr, "%.*s\n", (int)error_message.size, error_message.data);
      wasm_byte_vec_delete(&error_message);
    }
  else if (trap != NULL)
    {
      wasm_trap_message(trap, &error_message);
      fprintf(stderr, "%.*s\n", (int)error_message.size, error_message.data);
      wasm_byte_vec_delete(&error_message);
    }
  exit(1);
}

int
main(int argc, char *argv[])
{
  const char *wasm_path
      = (argc > 1) ? argv[1] : "build/osd.wasm";

  // Detect variant and resolution from WASM path
  const variant_config_t *variant = detect_variant(wasm_path);
  if (!variant)
    {
      fprintf(stderr, "error: could not detect variant from path: %s\n", wasm_path);
      fprintf(stderr, "expected one of: live_day, live_thermal, recording_day, recording_thermal\n");
      return 1;
    }

  const uint32_t WIDTH = variant->width;
  const uint32_t HEIGHT = variant->height;

  // Allow OUTPUT_PNG to be overridden via environment variable
  const char *output_png = getenv("OUTPUT_PNG");
  if (output_png == NULL) {
    output_png = OUTPUT_PNG;
  }

  printf("========================================\n");
  printf("  OSD WASM PNG Test Harness\n");
  printf("========================================\n");
  printf("\n");
  printf("WASM module: %s\n", wasm_path);
  printf("Variant: %s\n", variant->name);
  printf("Resolution: %dx%d\n", WIDTH, HEIGHT);
  printf("Output PNG: %s\n", output_png);
  printf("\n");

  // Initialize Wasmtime
  printf("Initializing Wasmtime...\n");
  wasm_engine_t *engine = wasm_engine_new();
  wasmtime_store_t *store = wasmtime_store_new(engine, NULL, NULL);
  wasmtime_context_t *context = wasmtime_store_context(store);

  // Load WASM module
  printf("Loading WASM module...\n");
  FILE *file = fopen(wasm_path, "rb");
  if (!file)
    {
      fprintf(stderr, "error: failed to open %s\n", wasm_path);
      return 1;
    }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0L, SEEK_SET);

  wasm_byte_vec_t wasm_bytes;
  wasm_byte_vec_new_uninitialized(&wasm_bytes, file_size);
  if (fread(wasm_bytes.data, 1, file_size, file) != file_size)
    {
      fprintf(stderr, "error: failed to read %s\n", wasm_path);
      fclose(file);
      return 1;
    }
  fclose(file);

  // Compile module
  printf("Compiling WASM module...\n");
  wasmtime_module_t *module = NULL;
  wasmtime_error_t *error = wasmtime_module_new(engine, (uint8_t *)wasm_bytes.data,
                                                  wasm_bytes.size, &module);
  if (error != NULL)
    {
      exit_with_error("failed to compile module", error, NULL);
    }
  wasm_byte_vec_delete(&wasm_bytes);

  // Configure WASI
  printf("Configuring WASI...\n");
  wasi_config_t *wasi_config = wasi_config_new();
  wasi_config_inherit_argv(wasi_config);
  wasi_config_inherit_env(wasi_config);
  wasi_config_inherit_stdin(wasi_config);
  wasi_config_inherit_stdout(wasi_config);
  wasi_config_inherit_stderr(wasi_config);

  // Preopen directories so WASM can load resources
  // Get absolute path to current directory
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
      fprintf(stderr, "ERROR: failed to get current directory\n");
      return 1;
    }
  printf("Preopening directory: %s → '.'\n", cwd);

  // Wasmtime v27+ uses 5-parameter API with permission flags
  // Grant READ-ONLY permissions (we only load resources, never write)
  // Use "." as guest path (WASI standard for current directory)
  if (!wasi_config_preopen_dir(wasi_config, cwd, ".",
                                1, // WASMTIME_WASI_DIR_PERMS_READ
                                1  // WASMTIME_WASI_FILE_PERMS_READ
                                ))
    {
      fprintf(stderr, "ERROR: failed to preopen directory\n");
      return 1;
    }
  printf("✓ Directory preopened as '.' with READ-ONLY permissions\n");

  // Preopen parent directory for accessing ../resources/
  char parent_dir[1024];
  snprintf(parent_dir, sizeof(parent_dir), "%s/..", cwd);
  printf("Preopening parent directory: %s → '..'\n", parent_dir);
  if (!wasi_config_preopen_dir(wasi_config, parent_dir, "..",
                                1, // WASMTIME_WASI_DIR_PERMS_READ
                                1  // WASMTIME_WASI_FILE_PERMS_READ
                                ))
    {
      fprintf(stderr, "ERROR: failed to preopen parent directory\n");
      return 1;
    }
  printf("✓ Parent directory preopened as '..' with READ-ONLY permissions\n");

  // Preopen resources directory with guest path "resources"
  // This allows WASM to access resources/fonts/... directly
  // Since we run from /workspace/, resources are at /workspace/resources
  char resources_dir[1024];
  snprintf(resources_dir, sizeof(resources_dir), "%s/resources", cwd);
  printf("Preopening resources directory: %s → 'resources'\n", resources_dir);
  if (!wasi_config_preopen_dir(wasi_config, resources_dir, "resources",
                                1, // WASMTIME_WASI_DIR_PERMS_READ
                                1  // WASMTIME_WASI_FILE_PERMS_READ
                                ))
    {
      fprintf(stderr, "ERROR: failed to preopen resources directory\n");
      return 1;
    }
  printf("✓ Resources directory preopened as 'resources' with READ-ONLY permissions\n");

  // Preopen build directory with guest path "build"
  // This allows WASM to access build/resources/*.json configs
  char build_dir[1024];
  snprintf(build_dir, sizeof(build_dir), "%s/build", cwd);
  printf("Preopening build directory: %s → 'build'\n", build_dir);
  if (!wasi_config_preopen_dir(wasi_config, build_dir, "build",
                                1, // WASMTIME_WASI_DIR_PERMS_READ
                                1  // WASMTIME_WASI_FILE_PERMS_READ
                                ))
    {
      fprintf(stderr, "ERROR: failed to preopen build directory\n");
      return 1;
    }
  printf("✓ Build directory preopened as 'build' with READ-ONLY permissions\n");

  // Create linker and define WASI FIRST, then set config on context
  printf("Instantiating module...\n");
  wasm_trap_t *trap = NULL;
  wasmtime_instance_t instance;
  wasmtime_linker_t *linker = wasmtime_linker_new(engine);
  error = wasmtime_linker_define_wasi(linker);
  if (error != NULL)
    {
      exit_with_error("failed to define WASI", error, NULL);
    }

  // NOW set WASI config on context (after defining WASI on linker)
  wasmtime_error_t *wasi_error = wasmtime_context_set_wasi(context, wasi_config);
  if (wasi_error != NULL)
    {
      exit_with_error("failed to configure WASI", wasi_error, NULL);
    }
  error = wasmtime_linker_instantiate(linker, context, module, &instance, &trap);
  if (error != NULL || trap != NULL)
    {
      exit_with_error("failed to instantiate module", error, trap);
    }
  wasmtime_linker_delete(linker);

  // Call _initialize() to set up WASI filesystem (reactor pattern)
  printf("Calling _initialize() to set up WASI filesystem...\n");
  wasmtime_extern_t initialize_extern;
  if (wasmtime_instance_export_get(context, &instance, "_initialize",
                                     strlen("_initialize"), &initialize_extern))
    {
      error = wasmtime_func_call(context, &initialize_extern.of.func, NULL, 0,
                                  NULL, 0, &trap);
      if (error != NULL || trap != NULL)
        {
          exit_with_error("failed to call _initialize", error, trap);
        }
      printf("✓ _initialize() called successfully\n");
    }
  else
    {
      printf("⚠️  _initialize() export not found (WASI filesystem may not work)\n");
    }

  // Get exports
  printf("Getting exported functions...\n");
  wasmtime_extern_t init_extern, render_extern, get_fb_extern;

  if (!wasmtime_instance_export_get(context, &instance, "wasm_osd_init",
                                     strlen("wasm_osd_init"), &init_extern))
    {
      fprintf(stderr, "error: failed to find wasm_osd_init export\n");
      return 1;
    }

  if (!wasmtime_instance_export_get(context, &instance, "wasm_osd_render",
                                     strlen("wasm_osd_render"),
                                     &render_extern))
    {
      fprintf(stderr, "error: failed to find wasm_osd_render export\n");
      return 1;
    }

  if (!wasmtime_instance_export_get(
          context, &instance, "wasm_osd_get_framebuffer",
          strlen("wasm_osd_get_framebuffer"), &get_fb_extern))
    {
      fprintf(stderr,
              "error: failed to find wasm_osd_get_framebuffer export\n");
      return 1;
    }

  // Call wasm_osd_init()
  printf("Calling wasm_osd_init()...\n");
  wasmtime_val_t results_init[1];
  error = wasmtime_func_call(context, &init_extern.of.func, NULL, 0,
                              results_init, 1, &trap);
  if (error != NULL || trap != NULL)
    {
      exit_with_error("failed to call wasm_osd_init", error, trap);
    }

  if (results_init[0].of.i32 != 0)
    {
      fprintf(stderr, "error: wasm_osd_init returned %d\n",
              results_init[0].of.i32);
      return 1;
    }
  printf("✓ Initialized successfully\n");

  // Load protobuf snapshot and update OSD state
  const char *proto_path = "test/proto_snapshot.bin";
  printf("Loading protobuf snapshot: %s\n", proto_path);

  // Variables for protobuf (needed for benchmark later)
  uint32_t proto_ptr = 0;
  uint32_t proto_size = 0;
  bool proto_loaded = false;
  wasmtime_extern_t update_state_extern;

  FILE *proto_file = fopen(proto_path, "rb");
  if (proto_file)
    {
      // Get file size
      fseek(proto_file, 0L, SEEK_END);
      size_t proto_file_size = ftell(proto_file);
      fseek(proto_file, 0L, SEEK_SET);
      proto_size = proto_file_size;

      printf("  Protobuf size: %u bytes\n", proto_size);

      // Allocate buffer and read
      uint8_t *proto_data = (uint8_t *)malloc(proto_size);
      if (!proto_data)
        {
          fprintf(stderr, "error: failed to allocate proto buffer\n");
          fclose(proto_file);
          return 1;
        }

      if (fread(proto_data, 1, proto_size, proto_file) != proto_size)
        {
          fprintf(stderr, "error: failed to read proto file\n");
          free(proto_data);
          fclose(proto_file);
          return 1;
        }
      fclose(proto_file);

      // Get WASM memory export
      wasmtime_extern_t memory_extern;
      if (!wasmtime_instance_export_get(context, &instance, "memory",
                                         strlen("memory"), &memory_extern))
        {
          fprintf(stderr, "error: failed to find memory export\n");
          free(proto_data);
          return 1;
        }

      wasmtime_memory_t *memory = &memory_extern.of.memory;
      uint8_t *memory_data = wasmtime_memory_data(context, memory);
      size_t memory_size = wasmtime_memory_data_size(context, memory);

      // Find a safe location in WASM memory to copy proto data
      // Framebuffer is 1920*1080*4 = 8,294,400 bytes (~8.3MB)
      // Place proto at 9MB offset to avoid overlap
      proto_ptr = 0x900000; // 9MB offset (safe beyond framebuffer)
      if (proto_ptr + proto_size > memory_size)
        {
          fprintf(stderr,
                  "error: not enough WASM memory for proto data\n");
          free(proto_data);
          return 1;
        }

      // Copy proto data into WASM memory
      memcpy(memory_data + proto_ptr, proto_data, proto_size);
      free(proto_data);
      printf("  ✓ Protobuf copied to WASM memory at 0x%08x\n", proto_ptr);

      // Get wasm_osd_update_state export
      if (!wasmtime_instance_export_get(
              context, &instance, "wasm_osd_update_state",
              strlen("wasm_osd_update_state"), &update_state_extern))
        {
          fprintf(stderr,
                  "error: failed to find wasm_osd_update_state export\n");
          return 1;
        }

      // Call wasm_osd_update_state(proto_ptr, proto_size)
      printf("Calling wasm_osd_update_state()...\n");
      wasmtime_val_t update_args[2];
      update_args[0].kind = WASMTIME_I32;
      update_args[0].of.i32 = proto_ptr;
      update_args[1].kind = WASMTIME_I32;
      update_args[1].of.i32 = proto_size;

      wasmtime_val_t update_results[1];
      error = wasmtime_func_call(context, &update_state_extern.of.func,
                                  update_args, 2, update_results, 1, &trap);
      if (error != NULL || trap != NULL)
        {
          exit_with_error("failed to call wasm_osd_update_state", error,
                          trap);
        }

      printf("  ✓ State updated (returned: %d)\n",
             update_results[0].of.i32);

      proto_loaded = true;
    }
  else
    {
      printf("  ⚠️  Protobuf snapshot not found, rendering with defaults\n");
    }

  // Call wasm_osd_render() and measure performance
  // (Checkerboard background is now drawn inside WASM)
  printf("Calling wasm_osd_render()...\n");

  // Warm-up render (JIT compilation, cache loading)
  wasmtime_val_t warmup_results[1];
  wasmtime_func_call(context, &render_extern.of.func, NULL, 0,
                      warmup_results, 1, NULL);

  // Performance measurement - run many times for accuracy
  const int ITERATIONS = 100;
  struct timespec start, end;

  if (proto_loaded)
    {
      // Benchmark with protobuf state updates (realistic production scenario)
      printf("  Benchmarking with protobuf state updates...\n");

      wasmtime_val_t update_args[2];
      update_args[0].kind = WASMTIME_I32;
      update_args[0].of.i32 = proto_ptr;
      update_args[1].kind = WASMTIME_I32;
      update_args[1].of.i32 = proto_size;

      clock_gettime(CLOCK_MONOTONIC, &start);

      for (int i = 0; i < ITERATIONS; i++)
        {
          // Update state (sets needs_render = true)
          wasmtime_val_t update_results[1];
          wasmtime_func_call(context, &update_state_extern.of.func,
                              update_args, 2, update_results, 1, NULL);

          // Render
          wasmtime_val_t results_render[1];
          error = wasmtime_func_call(context, &render_extern.of.func, NULL, 0,
                                      results_render, 1, &trap);
          if (error != NULL || trap != NULL)
            {
              exit_with_error("failed to call wasm_osd_render", error, trap);
            }
        }

      clock_gettime(CLOCK_MONOTONIC, &end);
    }
  else
    {
      // Benchmark with default state (no protobuf updates)
      printf("  Benchmarking with default state...\n");
      clock_gettime(CLOCK_MONOTONIC, &start);

      for (int i = 0; i < ITERATIONS; i++)
        {
          wasmtime_val_t results_render[1];
          error = wasmtime_func_call(context, &render_extern.of.func, NULL, 0,
                                      results_render, 1, &trap);
          if (error != NULL || trap != NULL)
            {
              exit_with_error("failed to call wasm_osd_render", error, trap);
            }
        }

      clock_gettime(CLOCK_MONOTONIC, &end);
    }

  // Calculate average time in microseconds
  long long start_ns = start.tv_sec * 1000000000LL + start.tv_nsec;
  long long end_ns = end.tv_sec * 1000000000LL + end.tv_nsec;
  long long total_ns = end_ns - start_ns;
  double total_ms = total_ns / 1000000.0;
  double avg_us = (total_ns / (double)ITERATIONS) / 1000.0;
  double avg_ms = avg_us / 1000.0;

  printf("✓ Render complete\n");
  printf("  Total time: %.1f ms for %d iterations\n", total_ms, ITERATIONS);
  printf("  Performance: %.2f μs/frame (%.4f ms/frame)\n", avg_us, avg_ms);

  // Show target achievement
  if (avg_ms < 1.0)
    {
      printf("  ✅ TARGET ACHIEVED: <1ms rendering (%.1f%% of target)\n",
             (avg_ms / 1.0) * 100.0);
      printf("  💡 SIMD optimization NOT needed - already fast enough!\n");
    }
  else
    {
      printf("  ⚠️  Above 1ms target - SIMD optimization recommended\n");
      printf("  📊 Potential SIMD speedup: 2-4× → ~%.1f μs (%.3f ms)\n",
             avg_us / 3.0, avg_ms / 3.0);
    }

  // Call wasm_osd_get_framebuffer()
  printf("Getting framebuffer pointer...\n");
  wasmtime_val_t results_fb[1];
  error = wasmtime_func_call(context, &get_fb_extern.of.func, NULL, 0,
                              results_fb, 1, &trap);
  if (error != NULL || trap != NULL)
    {
      exit_with_error("failed to call wasm_osd_get_framebuffer", error, trap);
    }

  uint32_t fb_ptr = results_fb[0].of.i32;
  printf("Framebuffer pointer: 0x%08x\n", fb_ptr);

  // Get WASM memory
  wasmtime_extern_t memory_extern;
  if (!wasmtime_instance_export_get(context, &instance, "memory",
                                     strlen("memory"), &memory_extern))
    {
      fprintf(stderr, "error: failed to find memory export\n");
      return 1;
    }

  wasmtime_memory_t *memory = &memory_extern.of.memory;
  uint8_t *memory_data = wasmtime_memory_data(context, memory);
  size_t memory_size = wasmtime_memory_data_size(context, memory);

  printf("WASM memory: %zu bytes\n", memory_size);

  // Verify framebuffer pointer is within memory bounds
  size_t fb_size = WIDTH * HEIGHT * 4; // RGBA
  if (fb_ptr + fb_size > memory_size)
    {
      fprintf(stderr,
              "error: framebuffer (0x%08x + %zu) exceeds memory size (%zu)\n",
              fb_ptr, fb_size, memory_size);
      return 1;
    }

  // Get framebuffer data (RGBA format from WASM)
  uint8_t *fb_data_wasm = memory_data + fb_ptr;

  // Generate checkerboard background in C
  printf("Generating checkerboard background...\n");
  uint8_t *fb_data_output = (uint8_t *)malloc(fb_size);
  if (!fb_data_output)
    {
      fprintf(stderr, "error: failed to allocate output buffer\n");
      return 1;
    }

  // Checkerboard parameters (matching WASM style)
  const int CHECKER_SIZE = 16; // 16x16 pixel squares
  const uint8_t COLOR1_R = 64, COLOR1_G = 64, COLOR1_B = 64;    // Dark gray
  const uint8_t COLOR2_R = 96, COLOR2_G = 96, COLOR2_B = 96;    // Light gray

  // Draw checkerboard pattern
  for (int y = 0; y < HEIGHT; y++)
    {
      for (int x = 0; x < WIDTH; x++)
        {
          int checker_x = x / CHECKER_SIZE;
          int checker_y = y / CHECKER_SIZE;
          bool is_dark = ((checker_x + checker_y) % 2) == 0;

          size_t idx = (y * WIDTH + x) * 4;
          fb_data_output[idx + 0] = is_dark ? COLOR1_R : COLOR2_R; // Red
          fb_data_output[idx + 1] = is_dark ? COLOR1_G : COLOR2_G; // Green
          fb_data_output[idx + 2] = is_dark ? COLOR1_B : COLOR2_B; // Blue
          fb_data_output[idx + 3] = 255;                           // Alpha (opaque)
        }
    }

  // Alpha blend WASM framebuffer onto checkerboard
  printf("Alpha blending WASM framebuffer onto checkerboard...\n");
  for (size_t i = 0; i < WIDTH * HEIGHT; i++)
    {
      // Extract RGBA from WASM framebuffer
      uint8_t r_src = fb_data_wasm[i * 4 + 0];
      uint8_t g_src = fb_data_wasm[i * 4 + 1];
      uint8_t b_src = fb_data_wasm[i * 4 + 2];
      uint8_t a_src = fb_data_wasm[i * 4 + 3];

      // Get background color from checkerboard
      uint8_t r_bg = fb_data_output[i * 4 + 0];
      uint8_t g_bg = fb_data_output[i * 4 + 1];
      uint8_t b_bg = fb_data_output[i * 4 + 2];

      // Alpha blend: result = src * alpha + bg * (1 - alpha)
      float alpha = a_src / 255.0f;
      fb_data_output[i * 4 + 0]
        = (uint8_t)(r_src * alpha + r_bg * (1.0f - alpha)); // Red
      fb_data_output[i * 4 + 1]
        = (uint8_t)(g_src * alpha + g_bg * (1.0f - alpha)); // Green
      fb_data_output[i * 4 + 2]
        = (uint8_t)(b_src * alpha + b_bg * (1.0f - alpha)); // Blue
      fb_data_output[i * 4 + 3] = 255; // Output is opaque
    }

  // Save as PNG
  printf("Writing PNG to %s...\n", output_png);
  if (!stbi_write_png(output_png, WIDTH, HEIGHT, 4, fb_data_output, WIDTH * 4))
    {
      fprintf(stderr, "error: failed to write PNG\n");
      free(fb_data_output);
      return 1;
    }

  free(fb_data_output);

  printf("✓ PNG saved successfully\n");
  printf("\n");
  printf("========================================\n");
  printf("✅ Test complete!\n");
  printf("========================================\n");
  printf("\n");
  printf("Output: %s\n", output_png);
  printf("\n");

  // Cleanup
  wasmtime_module_delete(module);
  wasmtime_store_delete(store);
  wasm_engine_delete(engine);

  return 0;
}
