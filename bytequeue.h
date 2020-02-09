#pragma once

#include <stdlib.h>
#include <stdio.h>

#include "diagnostic.h"

struct bytequeue {
	size_t pos, len, cap;
	char *array;
};

struct bytequeue make_bytequeue(void) {
	const size_t initsize = 8096;
	struct bytequeue q = {
		.pos = 0,
		.len = 0,
		.cap = initsize,
		.array = malloc(initsize)
	};
	return q;
}

static inline void bytequeue_put(struct bytequeue *q, char c) {
	if(q->len >= q->cap) {
		q->cap = q->cap * 4; // can never be 0
		q->array = realloc(q->array, q->cap);
	}
	q->array[q->len++] = c;
}

void bytequeue_rewind(struct bytequeue *q) {
}

static inline int bytequeue_get(struct bytequeue *q) {
	return q->pos >= q->len ? EOF : q->array[q->pos++];
}

void free_bytequeue(struct bytequeue q) {
	OPTIONAL_FREE(q.array);
}
