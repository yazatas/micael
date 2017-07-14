#include <ctype.h>

int isblank(int c)
{
	if (c == ' ' || c == '\t')
		return 1;
	return 0;
}
