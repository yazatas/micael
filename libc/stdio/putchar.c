#include <stdio.h>

#ifdef __is_libc
#include <unistd.h>
#else
#include <kernel/tty.h>
#endif

int putchar(int ic) {
	char c = (char) ic;
#ifdef __is_libc
	write(stdout, &c, 1);
#elif defined(__is_libk)
	term_putc(c);
#endif
	return ic;
}
