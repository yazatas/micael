#include <string.h>

char *strtok(char *str, const char *delim)
{ 
	static char *sp = NULL; 
	size_t i = 0;
	size_t len = strlen(delimiters);

	if (!str && !sp)
		return NULL;

	if (str && !sp)
		sp = str;

	char *p_start = sp;
	while (true) {
		for (i = 0; i < len; i++) {
			if (*p_start == delimiters[i]) {
				p_start++;
				break;
			}
		}

		if (i == len) {
			sp = p_start;
		    break;
		}
	}

	if (*sp == '\0') {
		sp = NULL;
		return sp;
	}

	while (*sp != '\0') {
		for (i = 0; i < len; i ++) {
			if (*sp == delimiters[i]) {
				*sp = '\0';
				break;
			}
		}

		sp++;
		if (i < len)
			break;
	}
	return p_start;
}
