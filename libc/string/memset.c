#include <string.h>

void *memset(void *buf, int c, size_t size)
{
	uint8_t *ptr = buf;
	while  (size--)
		*ptr++ = c;
	return buf;
}
