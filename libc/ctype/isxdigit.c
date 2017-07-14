#include <ctype.h>

int isxdigit(int c)
{
	if (isdigit(c) || isupper(c) || islower(c))
		return 1;
	return 0;
}
