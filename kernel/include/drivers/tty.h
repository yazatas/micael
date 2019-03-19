#ifndef __TTY_H__
#define __TTY_H__

#include <fs/char.h>

typedef struct tty {
    /* character driver for the tty */
    cdev_t *dev;

    /* pointer to 64kb of video memory
     *
     * each tty should have it's own video memory so that
     * two tty's can be used "concurrently" (running some long task in tty1
     * and while that task's running, switch to tty2 to run some other tasks) */
    void *vmem;

    /* order number of this tty
     *
     * numbering starts from 1 because tty0 is a special tty
     * always pointing to currently active tty */
    int num;

    /* name of the tty (f.ex. tty3) */
    char *name;
} tty_t;

/* allocate new tty object
 *
 * allocated tty can be controlled using this object
 * or by directly reading/writing from/to /dev/name
 * where name is the tty's name (tty + tty order number) */
tty_t *tty_init(void);

/* TODO: is this needed? */
void tty_switch(tty_t *tty);

void tty_putc(char c);
void tty_puts(char *data);

#endif /* __TTY_H__ */
