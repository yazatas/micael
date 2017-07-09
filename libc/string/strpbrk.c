#include <string.h>

char *strpbrk(const char *s, const char *accept)
{
	size_t s_len = strlen(s);
	size_t accept_len = strlen(accept);

	for (size_t i = 0; i < s_len; ++i) {
		for (size_t j = 0; j < accept_len; ++j) {
			if (s[i] == accept[j])
				return s + i;
		}
	}
	return NULL;
}
