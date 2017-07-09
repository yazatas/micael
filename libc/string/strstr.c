#include <string.h>

char *strstr(const char *haystack, const char *needle)
{
	for (; *haystack; haystack++) {
		const char *h, *n;
		for (h = haystack, n = needle; *h && *n && (*h == *n); ++h, ++n)
			;
		if (*n == '\0')
			return haystack;
	}
	return NULL;
}
