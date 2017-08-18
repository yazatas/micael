#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/tty.h>
#include <kernel/ssp.h>

/* TODO: this whole thing needs a rewrite 
 * and relocation to drivers directory 
 *
 * real vga when??? */

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static volatile uint16_t *vga_buffer = (uint16_t*) 0xc00B8000;

static uint8_t term_color;
static size_t term_row, term_col;

static inline void tty_entry(char c, uint8_t color, size_t x, size_t y)
{
	size_t index = y * VGA_WIDTH + x;
	vga_buffer[index] = c | (color << 8);
}

void tty_init_default(void)
{
	tty_init(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
}

void tty_init(int fg, int bg)
{
	term_row = term_col = 0;
	term_color = fg | bg << 4;

	for (size_t x = 0; x < VGA_WIDTH; x++) {
		for (size_t y = 0; y < VGA_HEIGHT; y++) {
			tty_entry(0x0, term_color, x, y);
		}
	}
}

void term_putc(char c)
{
	switch (c) {
		case '\n':
			term_row++;
			term_col = 0;
			break;

		default: {
			tty_entry(c, term_color, term_col, term_row);
			term_col++;
			break;
		}
	}

	if (term_col >= VGA_WIDTH) {
		term_col = 0;
		term_row++;
	}

	if (term_row >= VGA_HEIGHT) {
		for (size_t row = 1; row < VGA_HEIGHT; ++row) {
			for (size_t col = 0; col < VGA_WIDTH; ++col) {
				int prev = (VGA_WIDTH * row) + col - VGA_WIDTH;
				int cur  = (VGA_WIDTH * row) + col;
				vga_buffer[prev] = vga_buffer[cur];
			}
		}

		term_row = VGA_HEIGHT - 1;
		if (c == '\n') { /* clear last line */
			for (size_t col = 0; col < VGA_WIDTH; ++col) {
				size_t index = (VGA_WIDTH * term_row) + col;
				vga_buffer[index] = (0x0 | term_color << 8);
			}
		}
	}
}

void term_puts(char *data)
{
	size_t size = strlen(data);
	for (size_t i = 0; i < size; i++)
		term_putc(data[i]);
}
