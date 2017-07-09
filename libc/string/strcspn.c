#include <string.h>
#include <stdbool.h>

size_t strcspn(const char *s, const char *reject)
{
	size_t s_len = strlen(s);
	size_t reject_len = strlen(reject);
	size_t i, j;
	bool found = false;

	for (i = 0; i < s_len; ++i) {
		for (j = 0; j < reject_len; ++j) {
			if (s[i] == reject[j]) {
				found = true;
				break;
			}
			if (found)
				return i;
			found = false;
		}
	}
	return i;
}
