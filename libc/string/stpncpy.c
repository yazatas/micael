#include <string.h>

char *stpncpy(char *dest, const char *src, size_t n)
{
	size_t srclen = strlen(src);
	int k = -1;

	for (size_t i = 0; i < n; ++i) {
		if (i < srclen) {
			dest[i] = src[i];
		} else {
			if (k == -1)
				k = i;
			dest[i] = '\0';
		}
	}
	return (k == -1) ? &dest[n-1] : &dest[k];
}
