#pragma once

#include <string.h>
#include <stdlib.h>

#include "diagnostic.h"
#include "hash.h"
#include "largenum.h"

/**
 * This header implements a mapping from names to expressions
 */

struct label {
	const char *name; // must not be null
	const char *expr; // can be NULL
	unsigned constant;
	struct label *next; // chained hash table
} labelmap[64] = {0};

bool lookup_label(const char *name, struct label *result) {
	struct label *node = &labelmap[strhash(name) & 63];
	while(node && node->name)
		if(!strcmp(node->name, name))
			return (*result = *node), true;
		else
			node = node->next;
	return false;
}

static void set_label(const char *name, long double constant, const char *expr) {
	unsigned bucket = strhash(name) & 63;
	struct label newlabel = {.name = name, .expr = expr, .constant = constant};
	struct label *node = &labelmap[bucket];
	// loop until (node is null) OR (found right key)
	while(node && node->name && strcmp(node->name, name))
		node = node->next;
	if(node) {
		// found right key
		newlabel.next = node->next;
		*node = newlabel;
	} else {
		// we swap out the topmost element and move the
		// old topmost element to a malloc'ed location
		struct label *swap = malloc(sizeof(struct label));
		*swap = labelmap[bucket];
		newlabel.next = swap;
		labelmap[bucket] = newlabel;
	}
}

#define set_expr_label(n, e) do { set_label(n, 0, e); } while(0)
#define set_constant_label(n, c) do { set_label(n, c, NULL); } while(0)

void cleanup_labels(void) {
	for(unsigned i = 0; i < 64; i++) {
		struct label *label = &labelmap[i];
		OPTIONAL_FREE(label->name); OPTIONAL_FREE(label->expr);
		label = label->next;
		while(label && label->name) {
			struct label *next = label->next;
			OPTIONAL_FREE(label->name); OPTIONAL_FREE(label->expr);
			OPTIONAL_FREE(label);
			label = next;
		}
	}
}
