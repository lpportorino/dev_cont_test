#include "utils/math.h"

#include <math.h>

// WASI SDK strict mode workaround for math functions
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-function-declaration"

// ════════════════════════════════════════════════════════════
// ANGLE OPERATIONS IMPLEMENTATION
// ════════════════════════════════════════════════════════════

double
normalize_angle_360(double angle)
{
  // Use fmod to wrap angle to [0, 360) range
  angle = fmod(angle, 360.0);

  // Handle negative angles (fmod can return negative values)
  if (angle < 0.0)
    {
      angle += 360.0;
    }

  return angle;
}

double
normalize_angle_180(double angle)
{
  // First normalize to [0, 360)
  angle = normalize_angle_360(angle);

  // Then map to [-180, 180)
  if (angle >= 180.0)
    {
      angle -= 360.0;
    }

  return angle;
}

double
angle_difference(double angle1, double angle2)
{
  // Calculate raw difference
  double diff = angle1 - angle2;

  // Normalize to [-180, 180) range
  // This gives us the shortest angular distance
  return normalize_angle_180(diff);
}

// ════════════════════════════════════════════════════════════
// INTERPOLATION IMPLEMENTATION
// ════════════════════════════════════════════════════════════

double
inverse_lerp(double a, double b, double value)
{
  if (fabs(b - a) < MATH_EPSILON)
    return 0.0;
  return (value - a) / (b - a);
}

// ════════════════════════════════════════════════════════════
// FLOAT COMPARISON IMPLEMENTATION
// ════════════════════════════════════════════════════════════

bool
float_equals(float a, float b, float epsilon)
{
  return fabsf(a - b) < epsilon;
}

bool
double_equals(double a, double b, double epsilon)
{
  return fabs(a - b) < epsilon;
}

bool
float_is_zero(float value)
{
  return fabsf(value) < MATH_EPSILON;
}

bool
double_is_zero(double value)
{
  return fabs(value) < MATH_EPSILON;
}

#pragma clang diagnostic pop
