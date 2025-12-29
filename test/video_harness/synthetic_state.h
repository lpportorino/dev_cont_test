/**
 * @file synthetic_state.h
 * @brief Generate synthetic protobuf state sequences
 *
 * Creates animated state sequences for testing OSD rendering.
 * Animates compass (rotation), rotary speeds, and timestamp.
 */

#ifndef SYNTHETIC_STATE_H
#define SYNTHETIC_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// Forward declare protobuf type (defined in generated code)
typedef struct _ser_JonGUIState ser_JonGUIState;

/**
 * Animation types
 */
typedef enum
{
  ANIM_STATIC,      // Static orientation (no movement)
  ANIM_ROTATE_YAW,  // Rotate around yaw axis (azimuth 0-360°)
  ANIM_ROTATE_PITCH, // Rotate around pitch axis (elevation -90 to +90°)
  ANIM_ROTATE_ROLL, // Rotate around roll axis (bank -180 to +180°)
  ANIM_CIRCLE,      // Circular rotation (yaw + pitch combined)
  ANIM_SPEED_PULSE  // Pulsing speed indicators
} animation_type_t;

/**
 * Synthetic state generator context
 */
typedef struct
{
  // Animation parameters
  animation_type_t type;
  uint32_t total_frames;
  uint32_t fps;

  // Current state
  uint32_t current_frame;
  ser_JonGUIState *state;
  uint8_t *encoded_buffer; // Protobuf encoded bytes
  size_t buffer_size;

  // Animation state
  float phase; // Animation phase (0.0 to 1.0)
} synthetic_state_t;

/**
 * Create synthetic state generator
 *
 * @param type Animation type
 * @param duration_seconds Duration of animation in seconds
 * @param fps Frames per second
 * @return Initialized generator, or NULL on error
 */
synthetic_state_t *synthetic_state_create (animation_type_t type,
                                            float duration_seconds,
                                            uint32_t fps);

/**
 * Generate next frame state
 *
 * Updates internal state to next frame in animation sequence.
 *
 * @param gen Generator context
 * @return true if frame generated, false if sequence complete
 */
bool synthetic_state_next_frame (synthetic_state_t *gen);

/**
 * Get current encoded state
 *
 * Returns protobuf-encoded state bytes for current frame.
 *
 * @param gen Generator context
 * @param out_size Output: size of encoded data
 * @return Pointer to encoded bytes, valid until next call
 */
const uint8_t *synthetic_state_get_encoded (synthetic_state_t *gen,
                                             size_t *out_size);

/**
 * Reset generator to beginning
 *
 * @param gen Generator context
 */
void synthetic_state_reset (synthetic_state_t *gen);

/**
 * Cleanup and free generator
 *
 * @param gen Generator context
 */
void synthetic_state_destroy (synthetic_state_t *gen);

#endif // SYNTHETIC_STATE_H
