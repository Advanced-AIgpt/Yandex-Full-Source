/**
 * @file util/config.h
 *
 * @date 2014
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2014 Yandex. All rights reserved.
 */

#pragma once

#if defined(SREAL)

#define REAL float
#define BIGREAL float
#define ABS(x) fabsf(x)
#define COS(x) cosf(x)
#define EXP(x) expf(x)
#define LOG(x) logf(x)
#define SIN(x) sinf(x)
#define SQRT(x) sqrtf(x)
#define POW(x, y) powf(x, y)
#define ISINF(x) isinf(x)
#define ISNAN(x) isnan(x)

#elif defined(ACCUM_REAL)

/* Avoid including `stdfix.h` here, use the built-in `_Accum` type name. */
#define REAL _Accum
#define BIGREAL long long _Accum
//#define BIGREAL float
#define ABS(x) ((x) < 0k ? -(x) : (x))
#define COS(x) ((_Accum)cosf((float)(x)))
#define EXP(x) ((_Accum)expf((float)(x)))
#define LOG(x) ((_Accum)logf((float)(x)))
#define SIN(x) ((_Accum)sinf((float)(x)))
#define SQRT(x) ((_Accum)sqrtf((float)(x)))
#define POW(x, y) ((_Accum)powf((float)(x), (float)(y)))
#define ISINF(x) ((x) == __ACCUM_MIN__ || (x) == __ACCUM_MAX__)
#define ISNAN(x) false

#else

#define REAL double
#define BIGREAL double
#define ABS(x) fabs(x)
#define COS(x) cos(x)
#define EXP(x) exp(x)
#define LOG(x) log(x)
#define POW(x, y) pow(x, y)
#define SIN(x) sin(x)
#define SQRT(x) sqrt(x)
#define ISINF(x) isinf(x)
#define ISNAN(x) isnan(x)

#endif

// for kiss_fft
#define kiss_fft_scalar REAL

#if defined(_MSC_VER)
static char const *const PATH_SEP = "\\";
#else
static char const *const PATH_SEP = "/";
#endif

/* Adapt to different macros being defined depending on Android toolchain setup. */
#if defined(__ANDROID__) && !defined(ANDROID)
#  define ANDROID __ANDROID__
#endif
#if defined (_MSC_VER)
#  define inline /* MSVC does not understand inline in C */
#  pragma warning( disable: 4996 ) /* I want to use sprintf without fighting with the compiler. */
#endif
