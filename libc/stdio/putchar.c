#include <stdio.h>

#if defined(__is_libk)
#include <kernel/tty.h>
#else
#include <syscalls.h>
#endif

int putchar(int ic) {
	char c = (char) ic;
#if defined(__is_libc)
	char *tmp = &c;
	asm volatile ("mov $0, %eax");
	asm volatile ("mov %0, %%ebx" : "=m"((unsigned int)tmp));
	asm volatile ("mov $1, %ecx");
	asm volatile ("int $0x80");
#elif defined(__is_libk)
	term_putc(c);
#endif
	return ic;
}
