#include <ctype.h>

int isxdigit(int c)
{
	if (isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
		return 1;
	return 0;
}
