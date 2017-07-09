#include <string.h>

char *strrchr(const char *s, int c)
{
	size_t len = strlen(s);
	for (size_t i = len; i >= 0; --i) {
		if (s[i] == c)
			return &s[i];
	}
	return NULL;
}
