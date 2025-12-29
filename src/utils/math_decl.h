/**
 * @file math_decl.h
 * @brief Math function declarations for WASI SDK compatibility with cglm
 *
 * WASI SDK's math.h in strict C99 mode doesn't expose all float math functions.
 * This header provides explicit declarations needed by cglm library.
 *
 * IMPORTANT: Include this header BEFORE <cglm/cglm.h> to ensure math functions
 * are visible to cglm's inline implementations.
 */

#ifndef MATH_DECL_H
#define MATH_DECL_H

#include <math.h>

/* Float math functions (single precision) */
extern float sqrtf(float x);
extern float cosf(float x);
extern float sinf(float x);
extern float tanf(float x);
extern float atanf(float x);
extern float atan2f(float y, float x);
extern float asinf(float x);
extern float acosf(float x);
extern float fabsf(float x);
extern float fmodf(float x, float y);
extern float floorf(float x);
extern float fminf(float x, float y);
extern float fmaxf(float x, float y);
extern float modff(float x, float *iptr);
extern float powf(float x, float y);
extern float expf(float x);
extern float logf(float x);

/* Float classification functions (only declare if not macros) */
#ifndef isnan
extern int isnan(double x);
#endif
#ifndef isinf
extern int isinf(double x);
#endif
#ifndef isfinite
extern int isfinite(double x);
#endif

/* NAN constant */
#ifndef NAN
#define NAN (0.0 / 0.0)
#endif

/* Double math functions */
extern double fabs(double x);
extern double sqrt(double x);
extern double cbrt(double x);
extern double hypot(double x, double y);
extern double sin(double x);
extern double cos(double x);
extern double tan(double x);
extern double sinh(double x);
extern double cosh(double x);
extern double tanh(double x);
extern double asin(double x);
extern double acos(double x);
extern double atan(double x);
extern double atan2(double y, double x);
extern double pow(double x, double y);
extern double exp(double x);
extern double exp2(double x);
extern double log(double x);
extern double log2(double x);
extern double log10(double x);
extern double floor(double x);
extern double ceil(double x);
extern double round(double x);
extern double trunc(double x);
extern double fmod(double x, double y);
extern double remainder(double x, double y);
extern double fmin(double x, double y);
extern double fmax(double x, double y);
extern double ldexp(double x, int exp);
extern double frexp(double x, int *exp);
extern double modf(double x, double *iptr);
extern double copysign(double x, double y);

#endif /* MATH_DECL_H */
