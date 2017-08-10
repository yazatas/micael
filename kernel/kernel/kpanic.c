#include <kernel/tty.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>

void kpanic(const char *error)
{
	term_init(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
	kprint("kernel panic: fatal exception: %s!\n", error);
	
	while (1) { }
	__builtin_unreachable();
}
