#include <string.h>
#include <stdlib.h>

/* TODO: implement malloc! */
char *strdup(const char *s)
{
	size_t n = strlen(s), i;
#ifdef __is_libk
    char *new = kmalloc(n + 1);
#else
	char *new = malloc(n + 1);
#endif

	if (!new) {
		/* TODO: set errno */
		return NULL;
	}

	for (i = 0; i < n; ++i)
		new[i] = s[i];
	new[i] = '\0';

	return new;
}
