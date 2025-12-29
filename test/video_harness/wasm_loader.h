/**
 * @file wasm_loader.h
 * @brief WASM module loader using Wasmtime C API
 *
 * Loads OSD WASM modules and provides access to exported functions.
 * Based on the reference implementation but simplified for testing.
 */

#ifndef WASM_LOADER_H
#define WASM_LOADER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <wasmtime.h>

// WASM module context
typedef struct
{
  // Wasmtime components
  wasm_engine_t *engine;
  wasmtime_store_t *store;
  wasmtime_context_t *context;
  wasmtime_module_t *module;
  wasmtime_instance_t instance;

  // Exported functions
  wasmtime_func_t init_func;
  wasmtime_func_t update_state_func;
  wasmtime_func_t render_func;
  wasmtime_func_t get_framebuffer_func;
  wasmtime_func_t destroy_func;

  // Memory access
  wasmtime_memory_t memory;
  uint8_t *memory_data;
  size_t memory_size;

  // Framebuffer info
  uint32_t framebuffer_ptr;
  uint32_t framebuffer_width;
  uint32_t framebuffer_height;
} osd_wasm_module_t;

/**
 * Load WASM module from file
 *
 * @param wasm_path Path to .wasm file
 * @param width Expected framebuffer width
 * @param height Expected framebuffer height
 * @return Initialized module, or NULL on error
 */
osd_wasm_module_t *wasm_module_load (const char *wasm_path,
                                  uint32_t width,
                                  uint32_t height);

/**
 * Call wasm_osd_init()
 *
 * @param module WASM module
 * @return 0 on success, non-zero on error
 */
int wasm_module_init (osd_wasm_module_t *module);

/**
 * Call wasm_osd_update_state()
 *
 * @param module WASM module
 * @param state_data Protobuf state bytes
 * @param state_size Size of state data
 * @return 0 on success, non-zero on error
 */
int wasm_module_update_state (osd_wasm_module_t *module,
                               const uint8_t *state_data,
                               uint32_t state_size);

/**
 * Call wasm_osd_render()
 *
 * @param module WASM module
 * @return 1 if rendered, 0 if skipped, negative on error
 */
int wasm_module_render (osd_wasm_module_t *module);

/**
 * Get framebuffer pointer
 *
 * Returns pointer to RGBA framebuffer in WASM memory.
 *
 * @param module WASM module
 * @return Pointer to framebuffer, or NULL on error
 */
uint8_t *wasm_module_get_framebuffer (osd_wasm_module_t *module);

/**
 * Cleanup and free WASM module
 *
 * @param module WASM module to destroy
 */
void wasm_module_destroy (osd_wasm_module_t *module);

#endif // WASM_LOADER_H
