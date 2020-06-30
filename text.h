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
	return textfail = (string[0] == expected);
}

static int hex2int(char c) {
	if('0' <= c && c <= '9')
		return c - '0';
	if('A' <= c && c <= 'F')
		return c - 'A' + 10;
	if('a' <= c && c <= 'f')
		return c - 'a' + 10;
	return -1;
}

size_t scan_octet(const char *string, int *out) {
	int high, low;

	if((high = hex2int(string[0])) < 0 || !string[1]) {
		textfail = true;
		return 1;
	}
	if((low = hex2int(string[1])) < 0) {
		textfail = true;
		return 2;
	}
	*out = (high << 4) | low;
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

size_t scan_formatter(const char *string, /* output: */ const char **out_fmt, const char **out_expr) {
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

	if(!textfail) {
		*out_fmt = fmt;
		*out_expr = expr;
	}

	return string - initial_string;
}
