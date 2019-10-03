#pragma once

#ifndef _POSIX_C_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>

/* 'ssize_t' is defined by POSIX, not standard C */
typedef ptrdiff_t ssize_t;

/* a non-standard header, but usually available */
#ifdef __PTRDIFF_MAX__
#define SSIZE_MAX __PTRDIFF_MAX__
#else
/* not a good idea, but at this point I just don't care */
#define SSIZE_MAX LONG_MAX
#endif

/*
 * The 'getdelim' and 'getline' functions have been taken from a public
 * domain repository on GitHub: https://github.com/ivanrad/getline
 */

/*
 * getdelim(), getline() - read a delimited record from stream, ersatz implementation
 *
 * For more details, see: http://pubs.opengroup.org/onlinepubs/9699919799/functions/getline.html
 *
 */

ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream) {
	char *cur_pos, *new_lineptr;
	size_t new_lineptr_len;
	int c;

	if (lineptr == NULL || n == NULL || stream == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (*lineptr == NULL) {
		*n = 128; /* init len */
		if ((*lineptr = (char *)malloc(*n)) == NULL) {
			errno = ENOMEM;
			return -1;
		}
	}

	cur_pos = *lineptr;
	for (;;) {
		c = getc(stream);

		if (ferror(stream) || (c == EOF && cur_pos == *lineptr))
			return -1;

		if (c == EOF)
			break;

		if ((*lineptr + *n - cur_pos) < 2) {
			if (SSIZE_MAX / 2 < *n) {
#ifdef EOVERFLOW
				errno = EOVERFLOW;
#else
				errno = ERANGE; /* no EOVERFLOW defined */
#endif
				return -1;
			}
			new_lineptr_len = *n * 2;

			if ((new_lineptr = (char *)realloc(*lineptr, new_lineptr_len)) == NULL) {
				errno = ENOMEM;
				return -1;
			}
			cur_pos = new_lineptr + (cur_pos - *lineptr);
			*lineptr = new_lineptr;
			*n = new_lineptr_len;
		}

		*cur_pos++ = (char)c;

		if (c == delim)
			break;
	}

	*cur_pos = '\0';
	return (ssize_t)(cur_pos - *lineptr);
}

ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
	return getdelim(lineptr, n, '\n', stream);
}

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
