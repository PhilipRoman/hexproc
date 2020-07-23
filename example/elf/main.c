#include <stdio.h>

// defined in our created object file
extern char hello[];

int main() {
	puts(hello);
	return 0;
}
