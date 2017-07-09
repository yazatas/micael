#include <string.h>

void *memccpy(void *destptr, const void *srcptr, int c, size_t n)
{
	uint8_t *dest = destptr;
	const uint8_t *src = srcptr;
	size_t i;

	for (i = 0; i < n; ++i) {
		dst[i] = src[i];

		if (dst[i] == c)
			return destptr + i + 1;
	}
	return NULL;
}
