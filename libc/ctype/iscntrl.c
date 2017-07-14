#include <ctype.h>

int iscntrl(int c)
{
	if ((c >= 0x00 && c <= 0x1f) || c == 0x7f)
		return 1;
	return 0;
}
