/**
 * @file celestial_position.h
 * @brief Celestial body position calculations for OSD navball indicators
 *
 * This module uses the Astronomy Engine library to calculate the positions
 * of the Sun and Moon in horizontal coordinates (azimuth and altitude) for
 * a given observer location and time. These positions are then used to render
 * celestial body indicators on the navball overlay.
 *
 * COORDINATE SYSTEMS:
 * - Azimuth: 0° = North, 90° = East, 180° = South, 270° = West
 * - Altitude: +90° = zenith (straight up), 0° = horizon, -90° = nadir (straight
 * down)
 *
 * ALGORITHM:
 * Uses Astronomy Engine (cosinekitty/astronomy) which implements:
 * - VSOP87 planetary theory for high accuracy
 * - NOVAS C 3.1 algorithms
 * - Accuracy: ±1 arcminute
 *
 * USAGE:
 * 1. Call celestial_init() once during OSD initialization
 * 2. Call celestial_calculate() each frame to update sun/moon positions
 * 3. Call celestial_cleanup() during OSD shutdown
 *
 * @see https://github.com/cosinekitty/astronomy
 */

#ifndef CELESTIAL_POSITION_H
#define CELESTIAL_POSITION_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Horizontal position of a celestial body (sun or moon)
 *
 * This structure contains the azimuth and altitude of a celestial body
 * as seen from an observer's location on Earth.
 */
typedef struct
{
  double azimuth;  /**< Compass direction in degrees (0=North, 90=East) */
  double altitude; /**< Elevation above horizon in degrees (+= up, -= down) */
  bool valid;      /**< True if calculation succeeded */
} celestial_position_t;

/**
 * @brief Positions of both Sun and Moon
 */
typedef struct
{
  celestial_position_t sun;  /**< Sun position (azimuth, altitude) */
  celestial_position_t moon; /**< Moon position (azimuth, altitude) */
} celestial_positions_t;

/**
 * @brief Observer location on Earth
 *
 * This structure defines the geographic location of the observer,
 * which is used to calculate horizon-relative positions of celestial bodies.
 */
typedef struct
{
  double latitude;  /**< Degrees north (+) or south (-) of equator */
  double longitude; /**< Degrees east (+) or west (-) of prime meridian */
  double altitude;  /**< Meters above (+) or below (-) sea level */
} observer_location_t;

/**
 * @brief Initialize celestial calculation system
 *
 * Performs one-time initialization for the Astronomy Engine library.
 * Must be called before any other celestial_* functions.
 *
 * @return True on success, false on failure
 */
bool celestial_init(void);

/**
 * @brief Calculate Sun and Moon positions for given time and location
 *
 * This function computes the horizontal coordinates (azimuth and altitude)
 * of the Sun and Moon as seen from the observer's location at the specified
 * time.
 *
 * INPUTS:
 * - unix_timestamp: Seconds since Unix epoch (1970-01-01 00:00:00 UTC)
 * - observer: Observer's geographic location (lat, lon, altitude)
 *
 * OUTPUTS:
 * - Returns celestial_positions_t with sun and moon positions
 * - Each position includes azimuth (0-360°) and altitude (-90 to +90°)
 * - valid=false if calculation failed
 *
 * PERFORMANCE:
 * - Typical execution time: ~500 μs (dominated by VSOP87 calculations)
 * - Safe to call every frame
 *
 * @param unix_timestamp Seconds since 1970-01-01 00:00:00 UTC
 * @param observer Observer's location on Earth
 * @return Positions of sun and moon (azimuth, altitude)
 */
celestial_positions_t celestial_calculate(int64_t unix_timestamp,
                                          observer_location_t observer);

/**
 * @brief Cleanup celestial calculation system
 *
 * Frees any resources allocated by celestial_init().
 * Safe to call even if celestial_init() was never called.
 */
void celestial_cleanup(void);

/**
 * @brief Convert celestial azimuth/altitude to navball screen coordinates
 *
 * This function transforms a celestial body's position from horizontal
 * coordinates (azimuth, altitude) to a screen position on the navball.
 *
 * COORDINATE TRANSFORMATIONS:
 * 1. Horizontal (azimuth, altitude) → 3D unit vector in horizontal frame
 * 2. Apply inverse platform rotation (using platform azimuth, elevation, bank)
 * 3. Project onto navball sphere → screen (x, y)
 *
 * INPUTS:
 * - azimuth: Celestial azimuth in degrees (0=North, 90=East, 180=South,
 * 270=West)
 * - altitude: Celestial altitude in degrees (+90=zenith, 0=horizon,
 * -90=nadir)
 * - platform_azimuth: Platform heading in degrees
 * - platform_elevation: Platform pitch in degrees
 * - platform_bank: Platform roll in degrees
 * - navball_center_x: Navball center X coordinate (pixels)
 * - navball_center_y: Navball center Y coordinate (pixels)
 * - navball_radius: Navball radius (pixels)
 *
 * OUTPUTS:
 * - screen_x: Output X coordinate on navball (pixels)
 * - screen_y: Output Y coordinate on navball (pixels)
 *
 * RETURNS:
 * - true if celestial body is visible on navball (front hemisphere)
 * - false if celestial body is behind navball (back hemisphere)
 *
 * @param azimuth Celestial azimuth (degrees, 0=North)
 * @param altitude Celestial altitude (degrees, 0=horizon)
 * @param platform_azimuth Platform heading (degrees)
 * @param platform_elevation Platform pitch (degrees)
 * @param platform_bank Platform roll (degrees)
 * @param navball_center_x Navball center X (pixels)
 * @param navball_center_y Navball center Y (pixels)
 * @param navball_radius Navball radius (pixels)
 * @param screen_x Output screen X coordinate (pixels)
 * @param screen_y Output screen Y coordinate (pixels)
 * @return true if visible on front of navball, false if behind
 */
bool celestial_to_navball_coords(double azimuth,
                                 double altitude,
                                 double platform_azimuth,
                                 double platform_elevation,
                                 double platform_bank,
                                 int navball_center_x,
                                 int navball_center_y,
                                 int navball_radius,
                                 int *screen_x,
                                 int *screen_y);

#endif /* CELESTIAL_POSITION_H */
