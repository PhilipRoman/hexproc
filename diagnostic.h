#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <inttypes.h>

uint64_t line_number = 0;
const char *current_file_name = "<unknown>";

void report_error(const char *fmt, ...) {
	va_list v;
	va_start(v, fmt);

	fprintf(stderr, "%s:%"PRIu64"  ", current_file_name, line_number);
	vfprintf(stderr, fmt, v);
	fputc('\n', stderr);

	va_end(v);
}

#ifdef CLEANUP
#define OPTIONAL_FREE(pointer) free(pointer)
#else
#define OPTIONAL_FREE(pointer)
#endif
