#pragma once

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "strdup.h"
#include "diagnostic.h"
#include "formatter.h"

bool textfail;

bool scan_char(const char *string, char expected) {
	return string[0] == expected;
}

size_t scan_octet(const char *string, char *out) {
	out[0] = string[0];
	if(!string[1]) {
		report_error("Unfinished octet (starts with %c)", string[0]);
		return 1;
	}
	out[1] = string[1];
	return 2;
}

size_t scan_whitespace(const char *string) {
	size_t i = 0;
	while(isspace(string[i]))
		i++;
	return i;
}

size_t scan_balanced(const char *string, const char **out, const char *pattern) {
	char opening = pattern[0];
	char closing = pattern[1];
	if(string[0] != opening) {
		report_error("Balanced string does not start with '%c'", opening);
		textfail = true;
		return 0;
	}
	unsigned stack = 1;
	unsigned i = 1;
	while(stack != 0) {
		char c = string[i++];
		if(c == '\0') {
			report_error("Balanced string does not end with '%c'", closing);
			i--;
			break;
		}
		if(c == closing)
			stack--;
		else if(c == opening)
			stack++;
	}
	*out = strndup(string + 1, i - 2);
	return i;
}

size_t scan_quoted_string(const char *string) {
	if(string[0] != '"') {
		report_error("Expected quoted string, got '%c'", string[0]);
		textfail = true;
		return 0;
	}
	unsigned i = 1;
	while(1) {
		char c = string[i++];
		if(c == '\0') {
			report_error("Unfinished quoted string");
			i--;
			break;
		} else if(c == '"') {
			break;
		}
	}
	return i;
}

size_t scan_name(const char *string, const char **out) {
	for(size_t i = 0; ; i++) {
		char c = string[i];
		bool namechar = c == '.' || c == '_' || isalnum(c);
		if(!namechar) {
			if(i > 0)
				*out = strndup(string, i);
			else
				textfail = true;
			return i;
		}
	}
}

bool scan_line_marker(const char *line, uint64_t *linenum, const char **filename) {
	if(line[0] != '#')
		return false;
	line++;
	const char *endptr;
	long n = strtol(line, (char**)&endptr, 10);
	if(n < 0)
		return false;
	if(line == endptr) // failed to parse
		return false;
	line = endptr;
	line += scan_whitespace(line);
	if(line[0] != '"')
		return false;
	size_t quotedlength = scan_quoted_string(line);
	if(!quotedlength)
		return false;

	*linenum = n;
	*filename = strndup(line+1, quotedlength - 2);
	return true;
}

static inline size_t line_len(const char *line) {
	for(size_t i = 0; ; i++)
		if(line[i] == '\0' || line[i] == ';')
			return i;
}

void trim_end(const char *line, size_t *length) {
	size_t remaining = *length;
	while(remaining && isspace(line[remaining - 1]))
		remaining--;
	*length = remaining;
}

enum assign_mode {ASSIGN_LABEL, ASSIGN_LAZY, ASSIGN_IMMEDIATE};

size_t try_scan_assign(const char *line, const char **key, const char **value, enum assign_mode *mode) {
	size_t total_length = line_len(line);
	const char *line_start = line;
	line += scan_whitespace(line);

	const char *k;
	size_t key_length = scan_name(line, &k);
	if(!key_length) {
		return 0;
	}
	line += key_length;

	line += scan_whitespace(line);
	if(line[0] == '=') {
		line++;
		line += scan_whitespace(line);
		size_t remaining = line_len(line);
		trim_end(line, &remaining);
		*key = k;
		*value = strndup(line, remaining);
		*mode = ASSIGN_LAZY;
		return total_length;
	} else if(line[0] == ':' && line[1] == '=') {
		line += 2;
		line += scan_whitespace(line);
		size_t remaining = line_len(line);
		trim_end(line, &remaining);
		*key = k;
		*value = strndup(line, remaining);
		*mode = ASSIGN_IMMEDIATE;
		return total_length;
	} else if(line[0] == ':') {
		line++;
		*key = k;
		*value = NULL;
		*mode = ASSIGN_LABEL;
		return line - line_start;;
	} else {
		free((char*) k);
		return false;
	}
}

static struct formatter create_formatter(const char *fmt, const char *expr) {
	if(!fmt || !expr)
		report_error("Missing formatter or expression");
	struct formatter result = {
		.expr = expr,
		.datatype = HP_INT,
		.big_endian = true,
		.nbytes = 0,
	};

	bool use_default_size = true;
	bool use_default_endian = true;

	while(fmt[0]) {
		fmt += scan_whitespace(fmt);
		const char *attr;
		fmt += scan_name(fmt, &attr);
		if(!attr) {
			report_error("Expected formatter attribute");
			return result;
		}
		if(isdigit(attr[0])) {
			// we're parsing a numeric byte width
			result.nbytes = attr[0] - '0';
			if(result.nbytes > 8)
				report_error("Number of bytes (%d) can't be more than 8", (int) result.nbytes);
			else
				use_default_size = false;
		} else if(isalpha(attr[0])) {
			if(strcmp("LE", attr)==0) {
				result.big_endian = false; use_default_endian = false;
			} else if(strcmp("BE", attr)==0) {
				result.big_endian = true; use_default_endian = false;
			} else {
				result.datatype = resolve_datatype(attr);
			}
		}
		free((char*)attr);
		fmt += scan_whitespace(fmt);
		fmt += scan_char(fmt, ',');
	}

	double calc(const char *expr);

	if(use_default_size)
		result.nbytes = datatype_default_size(result.datatype);
	if(use_default_endian)
		result.big_endian = calc("hexproc.endian") != 0;
	return result;
}

size_t scan_formatter(const char *string, struct formatter *out) {
	const char *initial_string = string;

	const char *fmt = NULL;
	const char *expr = NULL;

	string += scan_balanced(string, &fmt, "[]");
	if(textfail)
		return 0;

	string += scan_whitespace(string);

	if(string[0] == '(')
		string += scan_balanced(string, &expr, "()");
	else
		string += scan_name(string, &expr);

	if(!textfail)
		*out = create_formatter(fmt, expr);

	free((char*) fmt);
	return string - initial_string;
}
