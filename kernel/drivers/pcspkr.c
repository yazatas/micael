#include <drivers/pcspkr.h>
#include <kernel/io.h>
#include <kernel/tick.h>

void spkr_play(unsigned ms)
{
    outb(0x43, 0xb6);
    outb(0x42, (uint8_t)(1193 >> 0));
    outb(0x42, (uint8_t)(1193 >> 8));

    /* and play the sound using the PC speaker */
    outb(0x61, inb(0x61) | 3);

    tick_wait(tick_ms_to_ticks(ms));

    /* stop the beep */
    outb(0x61, inb(0x61) & 0xfc);
}
