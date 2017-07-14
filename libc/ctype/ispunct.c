#include <ctype.h>

int ispunct(int c)
{
	if (c >= 0x21 && c <= 0x2f)
		return 1;
	else if (c >= 0x3a && c <= 0x40)
		return 1;
	else if (c >= 0x5b && c <= 0x60)
		return 1;
	if (c >= 0x7b && c <= 0x7e)
		return 1;
	return 0;
}
