#pragma once

#include <string.h>
#include <stdlib.h>

#include "error.h"

struct label {
	const char *name;
	const char *expr; // can be NULL
	unsigned constant;
};

struct label *labellist = NULL;
size_t labellist_len = 0;
size_t labellist_cap = 0;

static void add_label(const char *name, unsigned constant, const char *expr) {
	if(labellist_len >= labellist_cap) {
		if(labellist_cap)
			labellist_cap *= 3;
		else
			labellist_cap = 16;
		labellist = realloc(labellist, labellist_cap * sizeof(struct label));
	}
	struct label label = {
		.name = name,
		.expr = expr,
		.constant = constant,
	};
	labellist[labellist_len++] = label;
}

void add_expr_label(const char *name, const char *expr) {
	add_label(name, 0, expr);
}

void add_constant_label(const char *name, unsigned constant) {
	add_label(name, constant, NULL);
}

void cleanup_labels(void) {
	for(int i = 0; i < labellist_len; i++) {
		OPTIONAL_FREE((char*) labellist[i].name);
		OPTIONAL_FREE((char*) labellist[i].expr);
	}
	OPTIONAL_FREE(labellist);
}

struct label *lookup_label(const char *name) {
	for(int i = 0; i < labellist_len; i++) {
		if(strcmp(labellist[i].name, name) == 0) {
			return labellist + i;
		}
	}
	return NULL;
}
