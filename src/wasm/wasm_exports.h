// WASM Export Declarations
// Defines the public WASM API exposed to host (Wasmtime/JavaScript)
//
// ════════════════════════════════════════════════════════════
// NOTE TO OSD DEVELOPERS: You should NOT need to include this header.
// This is infrastructure code for the WASM/host boundary.
// ════════════════════════════════════════════════════════════

#ifndef WASM_EXPORTS_H
#define WASM_EXPORTS_H

#include <stdint.h>

// ════════════════════════════════════════════════════════════
// WASM EXPORT MACRO
// ════════════════════════════════════════════════════════════
//
// Marks functions for export from the WASM module.
// In WASI builds, this sets visibility to "default" so the linker
// exports the symbol. In non-WASI builds (testing), it's a no-op.

#ifdef WASI_BUILD
#define WASM_EXPORT __attribute__((visibility("default")))
#else
#define WASM_EXPORT
#endif

// ════════════════════════════════════════════════════════════
// PUBLIC WASM API
// ════════════════════════════════════════════════════════════
//
// These functions are called by the host (Wasmtime C API or JavaScript).
// They form the complete public interface of the OSD WASM module.

// Initialize OSD context
// Must be called once before any other functions.
// Returns: 0 on success, non-zero on error
WASM_EXPORT int wasm_osd_init(void);

// Update state from host
// Parameters:
//   state_ptr: Pointer to protobuf-encoded JonGUIState in WASM memory
//   state_size: Size of encoded protobuf in bytes
// Returns: 0 on success, non-zero on error
WASM_EXPORT int wasm_osd_update_state(uint32_t state_ptr, uint32_t state_size);

// Render OSD to framebuffer
// Call after wasm_osd_update_state() to render current state.
// Returns: 1 if rendered, 0 if skipped (no changes)
WASM_EXPORT int wasm_osd_render(void);

// Get framebuffer pointer
// Returns: Offset to RGBA framebuffer in WASM linear memory
// Size: width * height * 4 bytes (set during wasm_osd_init)
WASM_EXPORT uint32_t wasm_osd_get_framebuffer(void);

// Cleanup and free resources
// Returns: 0 on success
WASM_EXPORT int wasm_osd_destroy(void);

#endif // WASM_EXPORTS_H
