#pragma once

#include <limits.h>
#include <float.h>
#include <stdint.h>

// this header determines supported large numeric types
// (including non-standard ones) and their limits

// limits for float128 are not provided, because we can safely assume
// that range of largest float is greater than largest int

// As far as I know, Windows doesn't support any floats above 64-bit
// but gcc still claims it does, so do an explicit check

#if defined(__SIZEOF_FLOAT128__) && !defined(_WIN32)
#	define HAVE_HP_FLOAT128
	typedef __float128 hp_float128_t;
	typedef __float128 calc_float_t;
#elif defined(LDBL_MAX) && defined(LDBL_MIN) && !defined(_WIN32)
	typedef long double calc_float_t;
#elif defined(DBL_MAX) && defined(DBL_MIN)
	typedef double calc_float_t;
#endif

// if long double is a true 80-bit double
#if LDBL_MAX_EXP == 16384
#	define HAVE_HP_FLOAT80
	typedef long double hp_float80_t;
#endif

#ifdef __SIZEOF_INT128__
#	define HAVE_HP_INT128
// if we have int128, we probably have int64_t in stdint.h
#	define CALC_INT_MAX (( (__int128)INT64_MAX << 63) | UINT64_MAX)
#	define CALC_INT_MIN (-(__int128)INT64_MAX << 63)
	typedef __int128 hp_int128_t;
	typedef __int128 calc_int_t;
#else
#	define CALC_INT_MAX INTMAX_MAX
#	define CALC_INT_MIN INTMAX_MIN
	typedef intmax_t calc_int_t;
#endif
