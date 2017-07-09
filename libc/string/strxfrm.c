#include <string.h>

size_t strxfrm(char *dest, const char *src, size_t n)
{
	size_t src_len = strlen(src);
	if (n > src_len)
		strcpy(dest, src);
	return src_len;
}
