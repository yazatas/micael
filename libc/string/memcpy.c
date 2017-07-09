#include <string.h>

void *memcpy(void *dstptr, const void *srcptr, size_t size)
{
	uint8_t *dst = dstptr;
	const uint8_t *src = srcptr;

	for (size_t i = 0; i < size; i++)
		dst[i] = src[i];
	return dstptr;
}
