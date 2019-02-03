#include <string.h>

char *strchr(const char *s, int c)
{
	for (size_t i = 0; s[i] != '\0'; ++i) {
		if (s[i] == c)
			return (char *)&s[i];
	}
	return NULL;
}
