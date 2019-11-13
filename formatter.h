#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#include "error.h"

enum datatype {
	HP_FLOAT, HP_DOUBLE, HP_LONG, HP_INT, HP_SHORT, HP_BYTE
};

struct formatter {
	const char *expr;
	enum datatype datatype;
	bool big_endian;
	size_t nbytes;
};

struct formatter *formatqueue = NULL;
size_t formatqueue_cap = 0;
size_t formatqueue_len = 0;
size_t formatqueue_pos = 0;

void add_formatter(struct formatter fmt) {
	if(formatqueue_len >= formatqueue_cap) {
		if(formatqueue_cap)
			formatqueue_cap *= 3;
		else
			formatqueue_cap = 16;
		formatqueue = realloc(formatqueue, formatqueue_cap * sizeof(struct formatter));
	}
	formatqueue[formatqueue_len++] = fmt;
}

struct formatter default_formatter = {
	.expr = "NAN",
	.datatype = HP_INT,
	.big_endian = 1,
	.nbytes = 0,
};

struct formatter take_next_formatter(void) {
	if(formatqueue_pos >= formatqueue_len) {
		report_error("Formatter queue underflow");
		return default_formatter;
	} else {
		return formatqueue[formatqueue_pos++];
	}
}

void cleanup_formatters(void) {
	for(int i = 0; i < formatqueue_len; i++)
		OPTIONAL_FREE((char*) formatqueue[i].expr);
	OPTIONAL_FREE(formatqueue);
}

enum datatype resolve_datatype(const char *name) {
#define STR_EQ(a, b) (strcmp((a), (b)) == 0)
	if(STR_EQ(name, "long") || STR_EQ(name, "int64"))
		return HP_LONG;
	else if(STR_EQ(name, "int") || STR_EQ(name, "int32"))
		return HP_INT;
	else if(STR_EQ(name, "short") || STR_EQ(name, "int16"))
		return HP_SHORT;
	else if(STR_EQ(name, "byte") || STR_EQ(name, "char") || STR_EQ(name, "int8"))
		return HP_BYTE;
	else if(STR_EQ(name, "float") || STR_EQ(name, "ieee754_single"))
		return HP_FLOAT;
	else if(STR_EQ(name, "double") || STR_EQ(name, "ieee754_double"))
		return HP_DOUBLE;
	else {
		report_error("Unknown data type \"%s\"", name);
		return HP_INT;
	}
#undef STR_EQ
}

size_t datatype_default_size(enum datatype t) {
	switch(t) {
		case HP_FLOAT: return 4;
		case HP_DOUBLE: return 8;
		case HP_LONG: return 8;
		case HP_INT: return 4;
		case HP_SHORT: return 2;
		case HP_BYTE: return 1;
	}
}

void format_value(double value, struct formatter fmt, uint8_t *out, size_t *nbytes) {
	assert(sizeof(float) == 4);
	assert(sizeof(double) == 8);

	uint64_t v;
	switch(fmt.datatype) {
		case HP_LONG:
		case HP_INT:
		case HP_SHORT:
		case HP_BYTE: {
			if(!isfinite(value))
				report_error("%f cannot be converted to an integer", value);
			if(value > INT64_MAX) {
				// also includes positive infinity
				v = UINT64_MAX;
			} else if(isinf(value) < 0) {
				// negative infinity
				v = 0;
			} else if(value < 0) {
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
			memcpy(&v, &value, sizeof(double));
			break;
		}
	}
	uint8_t bytes[8];
	// input (big endian) 0x11_22_33_44_55
	// formatter: "[3,int,LE]"
	// result: 33 22 11
	if(fmt.big_endian) {
		unsigned shift = fmt.nbytes * 8;
		for(int i = 0; i < fmt.nbytes; i++) {
			shift -= 8;
			bytes[i] = (v >> shift) & 0xFF;
		}
	} else {
		unsigned shift = 0;
		for(int i = 0; i < fmt.nbytes; i++) {
			bytes[i] = (v >> shift) & 0xFF;
			shift += 8;
		}
	}
	memcpy(out, bytes, 8);
	*nbytes = fmt.nbytes;
}
