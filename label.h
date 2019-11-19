#pragma once

#include <string.h>
#include <stdlib.h>

#include "diagnostic.h"

struct label {
	const char *name;
	const char *expr; // can be NULL
	unsigned constant;
};

struct label *labellist = NULL;
size_t labellist_len = 0;
size_t labellist_cap = 0;

struct label *lookup_label(const char *name) {
	for(int i = 0; i < labellist_len; i++) {
		if(strcmp(labellist[i].name, name) == 0) {
			return labellist + i;
		}
	}
	return NULL;
}

static void add_label(const char *name, unsigned constant, const char *expr) {
	struct label *existing = lookup_label(name);
	if(existing) {
		existing->expr = expr;
		existing->constant = constant;
		return;
	}

	if(labellist_len >= labellist_cap) {
		labellist_cap = (labellist_cap == 0) ? 16 : labellist_cap * 3;
		labellist = realloc(labellist, labellist_cap * sizeof(labellist[0]));
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
