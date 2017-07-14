#include <ctype.h>

int isgraph(int c)
{
	if (c >= 21 && c <= 0x7e)
		return 1;
	return 0;
}
