#pragma once

#include <float.h>

// this header determines supported large numeric types
// including non-standard ones

// largest types are exported as 'hp_maxint_t' and 'hp_maxfloat_t'

// exact types are exported as
// hp_floatX_t (with HAVE_HP_FLOATX macros)
// and hp_intX_t (with HAVE_HP_INTX macros)

// for unsigned integers, add 'unsigned' manually



#ifdef __SIZEOF_FLOAT128__
#	define HAVE_HP_FLOAT128
	typedef __float128 hp_maxfloat_t;
	typedef __float128 hp_float128_t;
#else
	typedef long double hp_maxfloat_t;
#endif

#ifdef __SIZEOF_INT128__
#	define HAVE_HP_INT128
	typedef __int128 hp_maxint_t;
	typedef __int128 hp_int128_t;
#else
	typedef uintmax_t bigint_t;
#endif

// if long double is an 80-bit double
#if LDBL_MAX_EXP == 16384
#	define HAVE_HP_FLOAT80
	typedef long double hp_float80_t;
#endif
