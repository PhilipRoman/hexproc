#pragma once

#ifndef _POSIX_C_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stddef.h>

/*
 * The 'strdup' and 'strndup' functions have been taken from a public
 * domain code snippet on StackOverflow: https://stackoverflow.com/a/46013943
 */
char *strdup(const char *s) {
	size_t size = strlen(s) + 1;
	char *p = malloc(size);
	if (p != NULL) {
		memcpy(p, s, size);
	}
	return p;
}

char *strndup(const char *s, size_t n) {
	char *p = memchr(s, '\0', n);
	if (p != NULL)
		n = p - s;
	p = malloc(n + 1);
	if (p != NULL) {
		memcpy(p, s, n);
		p[n] = '\0';
	}
	return p;
}

#endif
