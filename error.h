#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>

int line_number = 1;

void report_error(const char *fmt, ...) {
	va_list v;
	va_start(v, fmt);

	fprintf(stderr, "line %d: ", line_number);
	vfprintf(stderr, fmt, v);
	fputc('\n', stderr);

	va_end(v);
}

#ifdef CLEANUP
#define OPTIONAL_FREE(pointer) free(pointer)
#else
#define OPTIONAL_FREE(pointer)
#endif
