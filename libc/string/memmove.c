#include <string.h>

void *memmove(void *dstptr, const void *srcptr, size_t size) {
	uint8_t *dst = dstptr;
	const uint8_t *src = srcptr;

	if (dst < src) {
		for (size_t i = 0; i < size; i++)
			dst[i] = src[i];
	} else {
		for (size_t i = size; i != 0; i--)
			dst[i-1] = src[i-1];
	}
	return dstptr;
}
