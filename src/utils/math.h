// Math Utilities
// Provides common mathematical operations for OSD rendering
//
// This module includes angle normalization, clamping, interpolation,
// and other frequently-used math functions.

#ifndef UTILS_MATH_H
#define UTILS_MATH_H

#include <math.h>
#include <stdbool.h>

// ════════════════════════════════════════════════════════════
// CONSTANTS
// ════════════════════════════════════════════════════════════

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MATH_EPSILON 1e-6f // Float comparison epsilon
#define MATH_DEG_TO_RAD (M_PI / 180.0)
#define MATH_RAD_TO_DEG (180.0 / M_PI)

// ════════════════════════════════════════════════════════════
// ANGLE OPERATIONS
// ════════════════════════════════════════════════════════════

// Normalize angle to [0, 360) degrees
//
// Examples:
//   normalize_angle_360(370.0) → 10.0
//   normalize_angle_360(-30.0) → 330.0
//   normalize_angle_360(720.0) → 0.0
double normalize_angle_360(double angle);

// Normalize angle to [-180, 180) degrees
//
// Examples:
//   normalize_angle_180(270.0)  → -90.0
//   normalize_angle_180(-190.0) → 170.0
//   normalize_angle_180(180.0)  → -180.0
double normalize_angle_180(double angle);

// Calculate smallest angular difference between two angles
// Result is in [-180, 180) range
//
// Examples:
//   angle_difference(10.0, 350.0)  → 20.0  (not -340.0)
//   angle_difference(350.0, 10.0)  → -20.0 (not 340.0)
//   angle_difference(180.0, -180.0) → 0.0
//
// Usage:
//   double diff = angle_difference(target_azimuth, camera_azimuth);
//   if (fabs(diff) < fov_half) { /* target in view */ }
double angle_difference(double angle1, double angle2);

// ════════════════════════════════════════════════════════════
// CLAMPING AND BOUNDS
// ════════════════════════════════════════════════════════════

// Clamp value to [min, max] range
//
// Examples:
//   clamp_double(5.0, 0.0, 10.0)  → 5.0
//   clamp_double(-5.0, 0.0, 10.0) → 0.0
//   clamp_double(15.0, 0.0, 10.0) → 10.0
static inline double
clamp_double(double value, double min, double max)
{
  if (value < min)
    return min;
  if (value > max)
    return max;
  return value;
}

static inline float
clamp_float(float value, float min, float max)
{
  if (value < min)
    return min;
  if (value > max)
    return max;
  return value;
}

static inline int
clamp_int(int value, int min, int max)
{
  if (value < min)
    return min;
  if (value > max)
    return max;
  return value;
}

// Min/max helpers
static inline double
min_double(double a, double b)
{
  return (a < b) ? a : b;
}

static inline double
max_double(double a, double b)
{
  return (a > b) ? a : b;
}

static inline float
min_float(float a, float b)
{
  return (a < b) ? a : b;
}

static inline float
max_float(float a, float b)
{
  return (a > b) ? a : b;
}

static inline int
min_int(int a, int b)
{
  return (a < b) ? a : b;
}

static inline int
max_int(int a, int b)
{
  return (a > b) ? a : b;
}

// ════════════════════════════════════════════════════════════
// INTERPOLATION
// ════════════════════════════════════════════════════════════

// Linear interpolation between a and b
//
// Parameters:
//   a: Start value
//   b: End value
//   t: Interpolation factor (0.0 = a, 1.0 = b)
//
// Examples:
//   lerp(0.0, 10.0, 0.5) → 5.0
//   lerp(0.0, 10.0, 0.0) → 0.0
//   lerp(0.0, 10.0, 1.0) → 10.0
//   lerp(100.0, 200.0, 0.25) → 125.0
//
// Note: t is not clamped - values outside [0,1] will extrapolate
static inline double
lerp(double a, double b, double t)
{
  return a + (b - a) * t;
}

// Linear interpolation with clamped t to [0, 1]
static inline double
lerp_clamped(double a, double b, double t)
{
  t = clamp_double(t, 0.0, 1.0);
  return a + (b - a) * t;
}

// Inverse lerp: given a value between a and b, return the t factor
//
// Examples:
//   inverse_lerp(0.0, 10.0, 5.0) → 0.5
//   inverse_lerp(0.0, 10.0, 2.5) → 0.25
//
// Returns 0.0 if a == b (to avoid division by zero)
double inverse_lerp(double a, double b, double value);

// Remap value from range [in_min, in_max] to [out_min, out_max]
//
// Examples:
//   remap(5.0, 0.0, 10.0, 0.0, 100.0) → 50.0
//   remap(2.5, 0.0, 10.0, -1.0, 1.0) → -0.5
static inline double
remap(
  double value, double in_min, double in_max, double out_min, double out_max)
{
  double t = inverse_lerp(in_min, in_max, value);
  return lerp(out_min, out_max, t);
}

// ════════════════════════════════════════════════════════════
// FLOAT COMPARISON
// ════════════════════════════════════════════════════════════

// Check if two floats are approximately equal (within epsilon)
bool float_equals(float a, float b, float epsilon);
bool double_equals(double a, double b, double epsilon);

// Check if float is approximately zero
bool float_is_zero(float value);
bool double_is_zero(double value);

// ════════════════════════════════════════════════════════════
// CONVERSIONS
// ════════════════════════════════════════════════════════════

// Convert degrees to radians
static inline double
deg_to_rad(double degrees)
{
  return degrees * MATH_DEG_TO_RAD;
}

// Convert radians to degrees
static inline double
rad_to_deg(double radians)
{
  return radians * MATH_RAD_TO_DEG;
}

#endif // UTILS_MATH_H
