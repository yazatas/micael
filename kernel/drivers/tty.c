#include <drivers/vbe.h>

/* for now this file is useless, but in the (near) future
 * it shall have bigger role especially in handling
 * the interaction with user */

void tty_putc(char c)
{
    vbe_put_char(c);
}

void tty_puts(char *data)
{
    if (!data)
        return;

    vbe_put_str(data);
}
