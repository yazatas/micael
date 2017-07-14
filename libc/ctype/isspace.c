#include <ctype.h>

int isspace(int c)
{
	if ((c >= 0x09 && c <= 0x0d) || c == 0x20)
		return 1;
	return 0;
}
