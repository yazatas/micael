#include <stdio.h>

#include <kernel/tty.h>

void kernel_main(void) {
	term_init();

	for (char c = 65; c <= 90; ++c) {
		putchar(c);

		if (c < 90)
			putchar('\n');
	}
}
