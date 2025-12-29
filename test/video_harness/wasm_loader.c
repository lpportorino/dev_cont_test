/**
 * @file wasm_loader.c
 * @brief WASM module loader implementation using Wasmtime
 */

#include "wasm_loader.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void
exit_with_error (const char *message,
                 wasmtime_error_t *error,
                 wasm_trap_t *trap)
{
  fprintf (stderr, "[WASM_LOADER] ERROR: %s\n", message);
  wasm_byte_vec_t error_message;
  if (error != NULL)
    {
      wasmtime_error_message (error, &error_message);
      fprintf (stderr, "%.*s\n", (int)error_message.size, error_message.data);
      wasm_byte_vec_delete (&error_message);
      wasmtime_error_delete (error);
    }
  else if (trap != NULL)
    {
      wasm_trap_message (trap, &error_message);
      fprintf (stderr, "%.*s\n", (int)error_message.size, error_message.data);
      wasm_byte_vec_delete (&error_message);
      wasm_trap_delete (trap);
    }
}

osd_wasm_module_t *
wasm_module_load (const char *wasm_path, uint32_t width, uint32_t height)
{
  printf ("[WASM_LOADER] Loading module: %s\n", wasm_path);
  printf ("[WASM_LOADER] Resolution: %ux%u\n", width, height);

  osd_wasm_module_t *module = calloc (1, sizeof (osd_wasm_module_t));
  if (!module)
    {
      fprintf (stderr, "[WASM_LOADER] Failed to allocate module\n");
      return NULL;
    }

  module->framebuffer_width = width;
  module->framebuffer_height = height;

  // Initialize Wasmtime
  module->engine = wasm_engine_new ();
  module->store = wasmtime_store_new (module->engine, NULL, NULL);
  module->context = wasmtime_store_context (module->store);

  // Load WASM file
  FILE *file = fopen (wasm_path, "rb");
  if (!file)
    {
      fprintf (stderr, "[WASM_LOADER] Failed to open %s\n", wasm_path);
      free (module);
      return NULL;
    }

  fseek (file, 0L, SEEK_END);
  size_t file_size = ftell (file);
  fseek (file, 0L, SEEK_SET);

  wasm_byte_vec_t wasm_bytes;
  wasm_byte_vec_new_uninitialized (&wasm_bytes, file_size);
  if (fread (wasm_bytes.data, 1, file_size, file) != file_size)
    {
      fprintf (stderr, "[WASM_LOADER] Failed to read %s\n", wasm_path);
      fclose (file);
      free (module);
      return NULL;
    }
  fclose (file);

  // Compile module
  wasmtime_error_t *error = wasmtime_module_new (
    module->engine, (uint8_t *)wasm_bytes.data, wasm_bytes.size,
    &module->module);
  if (error != NULL)
    {
      exit_with_error ("Failed to compile module", error, NULL);
      free (module);
      return NULL;
    }
  wasm_byte_vec_delete (&wasm_bytes);

  // Configure WASI
  wasi_config_t *wasi_config = wasi_config_new ();
  wasi_config_inherit_argv (wasi_config);
  wasi_config_inherit_env (wasi_config);
  wasi_config_inherit_stdin (wasi_config);
  wasi_config_inherit_stdout (wasi_config);
  wasi_config_inherit_stderr (wasi_config);

  // Preopen current directory and parent (for resource loading)
  char cwd[1024];
  if (getcwd (cwd, sizeof (cwd)) == NULL)
    {
      fprintf (stderr, "[WASM_LOADER] Failed to get current directory\n");
      free (module);
      return NULL;
    }

  // Preopen current directory as "."
  if (!wasi_config_preopen_dir (wasi_config, cwd, ".",
                                 1, // READ permission
                                 1  // FILE permission
                                 ))
    {
      fprintf (stderr, "[WASM_LOADER] Failed to preopen directory\n");
      free (module);
      return NULL;
    }

  // Preopen parent directory as ".."
  char parent_dir[2048];
  snprintf (parent_dir, sizeof (parent_dir), "%s/..", cwd);
  if (!wasi_config_preopen_dir (wasi_config, parent_dir, "..",
                                 1, // READ permission
                                 1  // FILE permission
                                 ))
    {
      fprintf (stderr, "[WASM_LOADER] Failed to preopen parent directory\n");
      free (module);
      return NULL;
    }

  // Preopen grandparent directory as "../.."
  char grandparent_dir[2048];
  snprintf (grandparent_dir, sizeof (grandparent_dir), "%s/../..", cwd);
  if (!wasi_config_preopen_dir (wasi_config, grandparent_dir, "../..",
                                 1, // READ permission
                                 1  // FILE permission
                                 ))
    {
      fprintf (stderr, "[WASM_LOADER] Failed to preopen grandparent directory\n");
      free (module);
      return NULL;
    }

  // Preopen resources directory with guest path "resources"
  // This allows WASM to access resources/fonts/... directly
  char resources_dir[2048];
  snprintf (resources_dir, sizeof (resources_dir), "%s/../../resources", cwd);
  if (!wasi_config_preopen_dir (wasi_config, resources_dir, "resources",
                                 1, // READ permission
                                 1  // FILE permission
                                 ))
    {
      fprintf (stderr, "[WASM_LOADER] Failed to preopen resources directory\n");
      free (module);
      return NULL;
    }

  // Preopen build directory with guest path "build"
  // This allows WASM to access build/resources/*.json configs
  char build_dir[2048];
  snprintf (build_dir, sizeof (build_dir), "%s/../../build", cwd);
  if (!wasi_config_preopen_dir (wasi_config, build_dir, "build",
                                 1, // READ permission
                                 1  // FILE permission
                                 ))
    {
      fprintf (stderr, "[WASM_LOADER] Failed to preopen build directory\n");
      free (module);
      return NULL;
    }

  // Create linker and define WASI
  wasmtime_linker_t *linker = wasmtime_linker_new (module->engine);
  error = wasmtime_linker_define_wasi (linker);
  if (error != NULL)
    {
      exit_with_error ("Failed to define WASI", error, NULL);
      free (module);
      return NULL;
    }

  // Set WASI config on context
  error = wasmtime_context_set_wasi (module->context, wasi_config);
  if (error != NULL)
    {
      exit_with_error ("Failed to configure WASI", error, NULL);
      free (module);
      return NULL;
    }

  // Instantiate module
  wasm_trap_t *trap = NULL;
  error = wasmtime_linker_instantiate (linker, module->context, module->module,
                                       &module->instance, &trap);
  if (error != NULL || trap != NULL)
    {
      exit_with_error ("Failed to instantiate module", error, trap);
      free (module);
      return NULL;
    }
  wasmtime_linker_delete (linker);

  // Call _initialize() (WASI reactor pattern)
  wasmtime_extern_t initialize_extern;
  if (wasmtime_instance_export_get (module->context, &module->instance,
                                     "_initialize", strlen ("_initialize"),
                                     &initialize_extern))
    {
      error = wasmtime_func_call (module->context,
                                   &initialize_extern.of.func, NULL, 0, NULL,
                                   0, &trap);
      if (error != NULL || trap != NULL)
        {
          exit_with_error ("Failed to call _initialize", error, trap);
        }
    }

  // Get exported functions
  if (!wasmtime_instance_export_get (module->context, &module->instance,
                                      "wasm_osd_init",
                                      strlen ("wasm_osd_init"),
                                      &initialize_extern))
    {
      fprintf (stderr, "[WASM_LOADER] wasm_osd_init export not found\n");
      free (module);
      return NULL;
    }
  module->init_func = initialize_extern.of.func;

  wasmtime_extern_t update_extern;
  if (!wasmtime_instance_export_get (module->context, &module->instance,
                                      "wasm_osd_update_state",
                                      strlen ("wasm_osd_update_state"),
                                      &update_extern))
    {
      fprintf (stderr,
               "[WASM_LOADER] wasm_osd_update_state export not found\n");
      free (module);
      return NULL;
    }
  module->update_state_func = update_extern.of.func;

  wasmtime_extern_t render_extern;
  if (!wasmtime_instance_export_get (module->context, &module->instance,
                                      "wasm_osd_render",
                                      strlen ("wasm_osd_render"),
                                      &render_extern))
    {
      fprintf (stderr, "[WASM_LOADER] wasm_osd_render export not found\n");
      free (module);
      return NULL;
    }
  module->render_func = render_extern.of.func;

  wasmtime_extern_t get_fb_extern;
  if (!wasmtime_instance_export_get (module->context, &module->instance,
                                      "wasm_osd_get_framebuffer",
                                      strlen ("wasm_osd_get_framebuffer"),
                                      &get_fb_extern))
    {
      fprintf (stderr,
               "[WASM_LOADER] wasm_osd_get_framebuffer export not found\n");
      free (module);
      return NULL;
    }
  module->get_framebuffer_func = get_fb_extern.of.func;

  // Get memory export
  wasmtime_extern_t memory_extern;
  if (!wasmtime_instance_export_get (module->context, &module->instance,
                                      "memory", strlen ("memory"),
                                      &memory_extern))
    {
      fprintf (stderr, "[WASM_LOADER] memory export not found\n");
      free (module);
      return NULL;
    }
  module->memory = memory_extern.of.memory;

  // Get memory data pointer
  module->memory_data
    = wasmtime_memory_data (module->context, &module->memory);
  module->memory_size
    = wasmtime_memory_data_size (module->context, &module->memory);

  printf ("[WASM_LOADER] Module loaded successfully\n");
  printf ("[WASM_LOADER] Memory size: %zu bytes\n", module->memory_size);

  return module;
}

int
wasm_module_init (osd_wasm_module_t *module)
{
  if (!module)
    return -1;

  wasmtime_val_t results[1];
  wasm_trap_t *trap = NULL;
  wasmtime_error_t *error = wasmtime_func_call (
    module->context, &module->init_func, NULL, 0, results, 1, &trap);

  if (error != NULL || trap != NULL)
    {
      exit_with_error ("wasm_osd_init() failed", error, trap);
      return -1;
    }

  int ret = results[0].of.i32;
  if (ret != 0)
    {
      fprintf (stderr, "[WASM_LOADER] wasm_osd_init() returned %d\n", ret);
      return ret;
    }

  printf ("[WASM_LOADER] OSD initialized\n");
  return 0;
}

int
wasm_module_update_state (osd_wasm_module_t *module,
                          const uint8_t *state_data,
                          uint32_t state_size)
{
  if (!module || !state_data)
    return -1;

  // Calculate safe proto offset: after framebuffer
  // Framebuffer is at framebuffer_ptr with size width*height*4
  uint32_t fb_size = module->framebuffer_width * module->framebuffer_height * 4;
  uint32_t fb_end = module->framebuffer_ptr + fb_size;

  // Align to 64KB boundary after framebuffer
  uint32_t proto_ptr = ((fb_end + 0xFFFF) & ~0xFFFF);

  // Verify bounds
  if (proto_ptr + state_size > module->memory_size)
    {
      fprintf (stderr, "[WASM_LOADER] State too large for WASM memory (need %u, have %zu)\n",
               proto_ptr + state_size, module->memory_size);
      return -1;
    }

  memcpy (module->memory_data + proto_ptr, state_data, state_size);

  // Call wasm_osd_update_state(proto_ptr, state_size)
  wasmtime_val_t args[2];
  args[0].kind = WASMTIME_I32;
  args[0].of.i32 = proto_ptr;
  args[1].kind = WASMTIME_I32;
  args[1].of.i32 = state_size;

  wasmtime_val_t results[1];
  wasm_trap_t *trap = NULL;
  wasmtime_error_t *error = wasmtime_func_call (
    module->context, &module->update_state_func, args, 2, results, 1, &trap);

  if (error != NULL || trap != NULL)
    {
      exit_with_error ("wasm_osd_update_state() failed", error, trap);
      return -1;
    }

  return results[0].of.i32;
}

int
wasm_module_render (osd_wasm_module_t *module)
{
  if (!module)
    return -1;

  wasmtime_val_t results[1];
  wasm_trap_t *trap = NULL;
  wasmtime_error_t *error = wasmtime_func_call (
    module->context, &module->render_func, NULL, 0, results, 1, &trap);

  if (error != NULL || trap != NULL)
    {
      exit_with_error ("wasm_osd_render() failed", error, trap);
      return -1;
    }

  return results[0].of.i32;
}

uint8_t *
wasm_module_get_framebuffer (osd_wasm_module_t *module)
{
  if (!module)
    return NULL;

  // Call wasm_osd_get_framebuffer() to get pointer
  wasmtime_val_t results[1];
  wasm_trap_t *trap = NULL;
  wasmtime_error_t *error
    = wasmtime_func_call (module->context, &module->get_framebuffer_func,
                          NULL, 0, results, 1, &trap);

  if (error != NULL || trap != NULL)
    {
      exit_with_error ("wasm_osd_get_framebuffer() failed", error, trap);
      return NULL;
    }

  module->framebuffer_ptr = results[0].of.i32;

  // Refresh memory pointer (in case it grew)
  module->memory_data
    = wasmtime_memory_data (module->context, &module->memory);

  return module->memory_data + module->framebuffer_ptr;
}

void
wasm_module_destroy (osd_wasm_module_t *module)
{
  if (!module)
    return;

  printf ("[WASM_LOADER] Destroying module\n");

  // Call wasm_osd_destroy() if available
  wasmtime_extern_t destroy_extern;
  if (wasmtime_instance_export_get (module->context, &module->instance,
                                     "wasm_osd_destroy",
                                     strlen ("wasm_osd_destroy"),
                                     &destroy_extern))
    {
      wasm_trap_t *trap = NULL;
      wasmtime_func_call (module->context, &destroy_extern.of.func, NULL, 0,
                          NULL, 0, &trap);
      if (trap != NULL)
        {
          wasm_trap_delete (trap);
        }
    }

  // Cleanup Wasmtime resources
  if (module->module)
    wasmtime_module_delete (module->module);
  if (module->store)
    wasmtime_store_delete (module->store);
  if (module->engine)
    wasm_engine_delete (module->engine);

  free (module);
}
