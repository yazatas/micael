#include <drivers/ps2.h>
#include <drivers/tty.h>
#include <kernel/irq.h>
#include <kernel/io.h>

unsigned char kbdus[128] =
{
    0, 27,
    
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',

    '-', '=', '\b', '\t',

    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    '\n',

    0, /* 29   - Control */

    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',

    0, /* Left shift */

    '\\', 'z', 'x', 'c', 'v', 'b', 'n',
    'm', ',', '.', '/', 
    
    0, /* Right shift */

    '*',
    0,  /* Alt */
    ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
    '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
    '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

static int char_read = 256;
static volatile int __read = 1;

static void ps2_read_char(void)
{
    uint8_t sc = inb(0x60);

    if (sc & 0x80) {
        /* key released */
    } else {
        char_read = kbdus[sc];
        __read = 0;
    }
}

unsigned char ps2_read_next(void)
{
    while (__read == 1)
        ;

    unsigned char ret = (unsigned char)char_read;
    __read = 1;

    return ret;
}

void ps2_init(void)
{
    irq_install_handler(ps2_read_char, 1);
}
