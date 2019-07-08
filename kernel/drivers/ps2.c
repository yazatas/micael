#include <drivers/ps2.h>
#include <drivers/tty.h>
#include <kernel/pic.h>
#include <kernel/io.h>
#include <stdbool.h>

enum {
    CONTROL  = 29,
    L_SHIFT  = 42,
    R_SHIFT  = 54,
    ALT      = 56,
    CAPSLOCK = 58,
    F1       = 59,
    F2       = 60,
    F3       = 61,
    F4       = 62,
    F5       = 63,
    F6       = 64,
    F7       = 65,
    F8       = 66,
    F9       = 67,
    F10      = 68,
    UP       = 72,
    LEFT     = 75,
    RIGHT    = 77,
    DOWN     = 80
};

unsigned char codes[256] = {
    [2]   = '1',     [3]   = '2',      [4]   = '3',     [5]  = '4',   [6]  = '5',
    [7]   = '6',     [8]   = '7',      [9]   = '8',     [10] = '9',   [11] = '0',
    [12]  = '-',     [13]  = '=',      [14]  = '\b',    [15] = '\t',  [16] = 'q',
    [17]  = 'w',     [18]  = 'e',      [19]  = 'r',     [20] = 't',   [21] = 'y',
    [22]  = 'u',     [23]  = 'i',      [24]  = 'o',     [25] = 'p',   [26] = '[',
    [27]  = ']',     [28]  = '\n',     [29]  = CONTROL, [30] = 'a',   [31] = 's',
    [32]  = 'd',     [33]  = 'f',      [34]  = 'g',     [35] = 'h',   [36] = 'j',
    [37]  = 'k',     [38]  = 'l',      [39]  = ';',     [40] = '\'',  [41] = '`',
    [42]  = L_SHIFT, [43]  = '\\',     [44]  = 'z',     [45] = 'x',   [46] = 'c',
    [47]  = 'v',     [48]  = 'b',      [49]  = 'n',     [50] = 'm',   [51] = ',',
    [52]  = '.',     [53]  = '/',      [54]  = R_SHIFT, [55] = '*',   [56] = ALT,
    [57]  = ' ',     [58]  = CAPSLOCK, [59]  = F1,      [60] = F2,    [61] = F3,
    [62]  = F4,      [63]  = F5,       [64]  = F6,      [65] = F7,    [66] = F8,
    [67]  = F9,      [68]  = F10,      [72]  = UP,      [75] = LEFT,  [77] = RIGHT,
    [78]  = '+',     [80]  = DOWN,

    /* key values when shift is down */
    [132] = '!',     [133] = '"',      [134] = '#',    [136] = '%',  [137] = '&',   
    [138] = '/',     [139] = '(',      [140] = ')',    [141] = '=',  [142] = '_',    
    [146] = 'Q',     [147] = 'W',      [148] = 'E',    [149] = 'R',  [150] = 'T',  
    [151] = 'Y',     [152] = 'U',      [153] = 'I',    [154] = 'O',  [155] = 'P',  
    [156] = '<',     [157] = '>',      [160] = 'A',    [161] = 'S',  [162] = 'D',    
    [163] = 'F',     [164] = 'G',      [165] = 'H',    [166] = 'J',  [167] = 'K',    
    [168] = 'L',     [174] = 'Z',      [175] = 'X',    [176] = 'C',  [177] = 'V',    
    [178] = 'B',     [179] = 'N',      [180] = 'M',    [181] = ';',  [182] = ':',     

};

static int char_read = 256;
static volatile int __read = 1;

static void ps2_read_char(void)
{
    static bool shift_down = false;

    uint8_t sc = inb(0x60);

    if (sc & 0x80) {
        sc &= ~0x80;

        if (sc == R_SHIFT || sc == L_SHIFT)
            shift_down = false;
    } else {
        if (sc == R_SHIFT || sc == L_SHIFT) {
            shift_down = true;
        } else {
            char_read = codes[shift_down ? sc + 130 : sc];
            __read = 0;
        }
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
