#include <ctype.h>

int isgraph(int c)
{
	if (c >= 0x21 && c <= 0x7e)
		return 1;
	return 0;
}
