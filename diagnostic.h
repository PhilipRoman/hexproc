#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

uint64_t line_number = 0;
const char *current_file_name = "<unknown>";
FILE *current_input = NULL;

#ifdef __GNUC__
__attribute((format (printf, 1, 2)))
#endif
void report_error(const char *fmt, ...) {
	va_list v;
	va_start(v, fmt);

	fprintf(stderr, "%s:%"PRIu64"  ", current_file_name, line_number);
	vfprintf(stderr, fmt, v);
	fputc('\n', stderr);

	va_end(v);
}

// Use OPTIONAL_FREE to free resources which wouldn't cause
// memory leaks, but should be freed to satisfy tools like Valgrind
#ifdef CLEANUP
#define OPTIONAL_FREE(pointer) free((char*)pointer)
#else
#define OPTIONAL_FREE(pointer) do {} while(0)
#endif
