#include <drivers/tty.h>
#include <drivers/vbe.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <mm/types.h>
#include <kernel/io.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <kernel/kpanic.h>

#define DISPLAY_WIDTH     1024
#define DISPLAY_HEIGHT     768
#define DISPLAY_BITDEPTH     8

#define DISPLAY_FG_COLOR  0x22
#define DISPLAY_BG_COLOR  0xff

#define ROWS_PER_BANK       64 /* pixel rows */
#define LAST_BANK           12

#define LINE_SIZE (DISPLAY_WIDTH * DISPLAY_BITDEPTH * 2)

static uint8_t *vga_mem  = NULL;
static uint8_t *font_map = NULL;

/* TODO: this is awful */
static uint32_t cur_x    = 0;
static uint32_t cur_y    = 0;
static uint32_t cur_bank = 0;

static void *line_buffer[2] = {
    NULL, NULL
};

static inline void vbe_write_reg(uint16_t index, uint16_t value)
{
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA,  value);
}

static inline uint16_t vbe_read_reg(uint16_t index)
{
    outw(VBE_DISPI_IOPORT_INDEX, index);
    return inw(VBE_DISPI_IOPORT_DATA);
}
 
static inline void vbe_set_bank(uint16_t num)
{
    vbe_write_reg(VBE_DISPI_INDEX_BANK, num);
    cur_bank = num;
}

static inline void vbe_putpixel(uint8_t color, uint32_t y, uint32_t x)
{
    vga_mem[(y * DISPLAY_WIDTH + x - 1)] = color;
}

static inline void vbe_putchar(unsigned char c, uint32_t y, uint32_t x, int fgcolor, int bgcolor)
{
    static const int mask[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
    unsigned char *glyph = font_map + (int)c * 16;
 
    for (int cy = 0; cy < 16; ++cy){
        for (int cx = 0; cx < 8; ++cx){
            vbe_putpixel(glyph[cy] & mask[cx] ? fgcolor : bgcolor, y + cy, x + 8 - cx);
        }
    }
}

static void vbe_get_font(void)
{
    /* clear even/odd mode */
    outw(0x3ce, 5);

    /* map VGA memory to 0A0000h */
    outw(0x3ce, 0x406);

    /* set bitplane 2 */
    outw(0x3ce, 0x402);

    /* clear even/odd mode */
    outw(0x3ce, 0x604);

    /* 8-bit font map SHOULD take 0x1000 (4096) bytes of space but because VGA
     * reserves space for 8x32 fonts, the font map "overflows" to 0xB000 and beyond 
     * We're not going to use the last 16 bytes of glyphs which means that one page
     * of memory is enough for us */
    unsigned long page = mmu_page_alloc(MM_ZONE_NORMAL);
    font_map           = mmu_p_to_v(page);
    vga_mem            = mmu_p_to_v(0xA0000);

#ifdef __x86_64__
    asm volatile("mov %[vga_ptr],  %%rsi\n\
                  mov %[font_ptr], %%rdi"
              :: [vga_ptr] "r" (vga_mem), [font_ptr] "r" (font_map));
#else
    asm volatile("mov %[vga_ptr],  %%esi\n\
                  mov %[font_ptr], %%edi"
              :: [vga_ptr] "r" (tmp_ptr), [font_ptr] "r" (font_map));
#endif

    /* 256 * 16 == 4096 bytes */
    for (int i = 0; i < 256; ++i) {
        asm volatile ("movsd");
        asm volatile ("movsd");
        asm volatile ("movsd");
        asm volatile ("movsd");

        /* discard the last 16 bytes of each glyph
         * we only need the first 16, see comment above */
#ifdef __x86_64__
        asm volatile ("add $16, %rsi");
#else
        asm volatile ("add $16, %esi");
#endif
    }

    /* restore VGA to normal state */
    outw(0x3ce, 0x302);
    outw(0x3ce, 0x204);
    outw(0x3ce, 0x1005);
    outw(0x3ce, 0xe06);
}

void vbe_init(void)
{
    vga_mem        = (uint8_t *)mmu_p_to_v(0xA0000);
    line_buffer[0] = mmu_p_to_v(mmu_block_alloc(MM_ZONE_NORMAL, 2));
    line_buffer[1] = mmu_p_to_v(mmu_block_alloc(MM_ZONE_NORMAL, 2));

    vbe_get_font();

    /* set video mode */
    vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write_reg(VBE_DISPI_INDEX_XRES,   DISPLAY_WIDTH);
    vbe_write_reg(VBE_DISPI_INDEX_YRES,   DISPLAY_HEIGHT);
    vbe_write_reg(VBE_DISPI_INDEX_BPP,    DISPLAY_BITDEPTH);
    vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    /* install new tty print routines */
    tty_install_putc(vbe_put_char);
    tty_install_puts(vbe_put_str);

    /* set all video memory to 0xff (black color) */
    vbe_clear_screen();
    vbe_set_bank(0);
}

void vbe_clear_screen(void)
{
    for (size_t bank = 0; bank < LAST_BANK; ++bank) {
        vbe_set_bank(bank);
        kmemset(vga_mem, 0xff, PAGE_SIZE * 16);
    }
}

void vbe_put_pixel(uint8_t color, uint32_t y, uint32_t x)
{
    vbe_putpixel(color, y, x);
}

void vbe_put_char(char c)
{
    if (vga_mem == NULL)
        return;

    switch (c) {
        case '\n':
            cur_y += DISPLAY_BITDEPTH * 2;

            /* TODO: explain this */
            if (cur_bank == LAST_BANK) {
                cur_y += DISPLAY_BITDEPTH * 2 * 3;
            }
            cur_x = 0;
            break;

        case '\b':
            vbe_putchar(0, cur_y, cur_x - DISPLAY_BITDEPTH, DISPLAY_FG_COLOR, DISPLAY_BG_COLOR);
            cur_x -= DISPLAY_BITDEPTH;
            break;

        case '\t':
            for (int i = 0; i < 4; ++i) {
                vbe_putchar(' ', cur_y, cur_x, DISPLAY_FG_COLOR, DISPLAY_BG_COLOR);
                cur_x += DISPLAY_BITDEPTH;
            }
            break;

        default:
            vbe_putchar(c, cur_y, cur_x, DISPLAY_FG_COLOR, DISPLAY_BG_COLOR);
            cur_x += DISPLAY_BITDEPTH;
            break;
    }

    /* end of line */
    if (cur_x >= DISPLAY_WIDTH) {
        cur_x = 0;
        cur_y += DISPLAY_BITDEPTH * 2;
    }

    /* end of bank */
    if (cur_y >= ROWS_PER_BANK) {
        if (cur_bank < LAST_BANK) {
            cur_x = cur_y = 0;
            vbe_set_bank(cur_bank + 1);
            goto end;
        }

        /* we're in last bank, shift all rows up by one
         * and place the write cursor to last line  of the last bank */
        for (int bank = cur_bank; bank >= 0; --bank) {
            /* copy the first line of bank to memory */
            kmemcpy(line_buffer[0], vga_mem, LINE_SIZE);

            /* shift all rows up by one */
            for (size_t i = 0; i < 3; ++i) {
                kmemset(vga_mem + i * LINE_SIZE, 0x0, LINE_SIZE);
                kmemcpy(vga_mem + i * LINE_SIZE, vga_mem + (i + 1) * LINE_SIZE, LINE_SIZE);
            }

            /* copy the first row of last bank to this bank */
            if (cur_bank != LAST_BANK)
                kmemcpy(vga_mem + 3 * LINE_SIZE, line_buffer[1], LINE_SIZE);

            /* make a copy of the first line of this bank */
            kmemcpy(line_buffer[1], line_buffer[0], LINE_SIZE); 
            vbe_set_bank(cur_bank - 1);
        }

        /* scrolling done */
        cur_x = cur_y = 0;
        vbe_set_bank(LAST_BANK);
    }

end:
    /* clear the new line */
    if (c == '\n') {
        kmemset(vga_mem + cur_y * DISPLAY_WIDTH, 0, LINE_SIZE);
    }
}

void vbe_put_str(char *s)
{
    if (!s)
        return;

    size_t len = kstrlen(s);

    for (size_t i = 0; i < len; ++i) {
        vbe_put_char(s[i]);
    }
}
