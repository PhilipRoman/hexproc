#pragma once

#include <stdlib.h>
#include <stdbool.h>

#include "diagnostic.h"

struct sourcemap_entry {
	size_t index;
	enum sourcemap_action {
		SOURCE_STRING,
		SOURCE_FORMATTER,
		SOURCE_NEWLINE,
		SOURCE_END /* marks the end of a token */
	} action;
} *sourcemap;

size_t sourcemap_pos, sourcemap_len, sourcemap_cap;

void add_sourcemap_entry(size_t index, int action) {
	if(sourcemap_len >= sourcemap_cap) {
		sourcemap_cap = (sourcemap_cap == 0) ? 16 : sourcemap_cap * 3;
		sourcemap = realloc(sourcemap, sourcemap_cap * sizeof(sourcemap[0]));
	}
	struct sourcemap_entry s = {.index = index, .action = action};
	sourcemap[sourcemap_len++] = s;
}

size_t next_sourcemap_index(void) {
	return sourcemap_pos < sourcemap_len
		? sourcemap[sourcemap_pos].index
		: (size_t)-1;
}

enum sourcemap_action take_next_sourcemap_action(void) {
	return sourcemap_pos < sourcemap_len
		? sourcemap[sourcemap_pos++].action
		: (report_error("Source map underflow"), SOURCE_END);
}

void cleanup_sourcemap(void) {
	OPTIONAL_FREE(sourcemap);
}
