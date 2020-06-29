#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#include "diagnostic.h"

enum datatype {
	HP_INT, HP_FLOAT, HP_DOUBLE
};

struct formatter {
	enum datatype datatype : 4;
	size_t nbytes : 6;
	enum {
		ENDIAN_DEFAULT, ENDIAN_BIG, ENDIAN_LITTLE
	} endian : 2;
	const char *expr;
};

struct formatter *formatqueue = NULL;
size_t formatqueue_cap = 0;
size_t formatqueue_len = 0;
size_t formatqueue_pos = 0;

void add_formatter(struct formatter fmt) {
	if(formatqueue_len >= formatqueue_cap) {
		formatqueue_cap = (formatqueue_cap == 0) ? 16 : formatqueue_cap * 3;
		formatqueue = realloc(formatqueue, formatqueue_cap * sizeof(formatqueue[0]));
	}
	formatqueue[formatqueue_len++] = fmt;
}

bool take_next_formatter(struct formatter *out) {
	return (formatqueue_pos >= formatqueue_len)
		? (report_error("Formatter queue underflow"), false)
		: (*out = formatqueue[formatqueue_pos++], true);
}

void cleanup_formatters(void) {
	for(unsigned i = 0; i < formatqueue_len; i++)
		OPTIONAL_FREE(formatqueue[i].expr);
	OPTIONAL_FREE(formatqueue);
}

const struct {
	char name[32];
	// a dummy formatter without expr or endian
	struct formatter format;
} typemap[] = {
	{"byte", {HP_INT, 1}},
	{"int8", {HP_INT, 1}},
	{"char", {HP_INT, 1}},

	{"short", {HP_INT, 2}},
	{"int16", {HP_INT, 2}},

	{"int", {HP_INT, 4}},
	{"int32", {HP_INT, 4}},

	{"long", {HP_INT, 8}},
	{"int64", {HP_INT, 8}},

	{"ieee754_single", {HP_FLOAT, 4}},
	{"float", {HP_FLOAT, 4}},
	{"ieee754_double", {HP_DOUBLE, 8}},
	{"double", {HP_DOUBLE, 8}},
	{{0}, {0}}
};

bool resolve_datatype(const char *name, struct formatter *out) {
	for(unsigned i = 0; typemap[i].name[0]; i++)
		if(!strcmp(name, typemap[i].name))
			return (*out = typemap[i].format), true;
	return false;
}

void format_value(long double value, struct formatter fmt, uint8_t out[static 8]) {
	uint64_t v;
	switch(fmt.datatype) {
		case HP_INT: {
			if(!isfinite(value))
				report_error("%Lf cannot be converted to an integer", value);
			if(value > INT64_MAX) {
				// also includes positive infinity
				v = UINT64_MAX;
			} else if(isinf(value) < 0) {
				// negative infinity
				v = 0;
			} else if(isnan(value)) {
				v = 0;
			} else {
				v = (uint64_t) value;
			}
			break;
		}
		case HP_FLOAT: {
			float f = (float) value;
			memcpy(&v, &f, sizeof(float));
			break;
		}
		case HP_DOUBLE: {
			double d = (double) value;
			memcpy(&v, &d, sizeof(double));
			break;
		}
	}
	uint8_t bytes[fmt.nbytes];
	// input (big endian) 0x11_22_33_44_55
	// formatter: "[3,int,LE]"
	// result: 33 22 11
	if(fmt.endian == 0)
		report_error("Internal error: endian not specified in formatter");
	if(fmt.endian == ENDIAN_BIG) {
		unsigned shift = fmt.nbytes * 8;
		for(unsigned i = 0; i < fmt.nbytes; i++) {
			shift -= 8;
			bytes[i] = (v >> shift) & 0xFF;
		}
	} else {
		unsigned shift = 0;
		for(unsigned i = 0; i < fmt.nbytes; i++) {
			bytes[i] = (v >> shift) & 0xFF;
			shift += 8;
		}
	}
	memcpy(out, bytes, fmt.nbytes);
}
