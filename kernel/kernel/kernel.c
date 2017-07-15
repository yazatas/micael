#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>

void kernel_main(void) {
	gdt_init();
	term_init();

	puts("haloust");

	/* for (char c = 65; c <= 90; ++c) { */
	/* 	putchar(c); */

	/* 	if (c < 90) */
	/* 		putchar('\n'); */
	/* } */
}
