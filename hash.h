#pragma once

unsigned strhash(const char *str) {
	unsigned hash = 1;
	for(size_t i = 0; str[i]; i++)
		hash = 92821 * hash + str[i];
	return hash;
}
