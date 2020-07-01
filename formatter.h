#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#include "diagnostic.h"
#include "calc.h"
#include "text.h"

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

static struct formatter create_formatter(const char *fmt, const char *expr) {
	if(!fmt || !expr)
		report_error("Missing formatter or expression");
	struct formatter blueprint = {0};
	int custom_size = -1;
	unsigned endian = 0; // see formatter.h
	while(fmt[0]) {
		fmt += scan_whitespace(fmt);
		const char *attr = NULL;
		fmt += scan_name(fmt, &attr);
		if(!attr) {
			report_error("Expected formatter attribute");
			return blueprint;
		}
		if(isdigit(attr[0])) {
			// we're parsing a numeric byte width
			char *numend;
			custom_size = strtol(attr, &numend, 0);
			if(strlen(attr) != numend-attr)
				report_error("Ignoring trailing characters in formatter size");
			if(custom_size > sizeof(calc_int_t)) {
				report_error("Number of bytes (%d) can't be more than %d", (int) custom_size, (int)sizeof(calc_int_t));
				custom_size = sizeof(calc_int_t);
			}
		} else if(isalpha(attr[0])) {
			if(strcmp("LE", attr)==0)
				endian = ENDIAN_LITTLE;
			else if(strcmp("BE", attr)==0)
				endian = ENDIAN_BIG;
			else if(!resolve_datatype(attr, &blueprint))
				report_error("Unknown data type: \"%s\"", attr);
		}
		free((char*)attr);
		fmt += scan_whitespace(fmt);
		fmt += scan_char(fmt, ',');
	}
	struct formatter result = {
		.expr = expr,
		.datatype = HP_INT,
		.nbytes = 1,
	};

	if(blueprint.nbytes /* if blueprint has been set */) {
		result.nbytes = blueprint.nbytes;
		result.datatype = blueprint.datatype;
		result.endian = blueprint.endian;
	}

	if(endian == 0 && blueprint.endian != 0)
		endian = blueprint.endian;
	if(endian == 0)
		endian = calc("hexproc.endian") == 0 ? ENDIAN_LITTLE : ENDIAN_BIG;
	result.endian = endian;

	if(custom_size >= 0)
		result.nbytes = custom_size;
	return result;
}

void format_value(calc_float_t value, struct formatter fmt, uint8_t *out /* must have space for at least 'fmt.nbytes' bytes */) {
	calc_int_t v;
	switch(fmt.datatype) {
		case HP_INT: {
			v = to_integer(value);
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
	// input (big endian) 0x11_22_33_44_55
	// formatter: "[3,int,LE]"
	// result: 33 22 11
	uint8_t bytes[fmt.nbytes];
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
