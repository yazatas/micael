#include <kernel/tty.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>

void kpanic(const char *error)
{
	asm ("cli");
	tty_init(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
	kdebug("kernel panic: fatal exception: %s!\n", error);
	
	while (1) { }
	__builtin_unreachable();
}
