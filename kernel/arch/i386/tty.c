#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/tty.h>
#include <kernel/ssp.h>

#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static volatile uint16_t *vga_buffer = (uint16_t*) 0xB8000;

static size_t term_row;
static size_t term_col;
static uint8_t term_color;

static inline void term_putentryat(char c, uint8_t color, size_t x, size_t y)
{
	const size_t index = y * VGA_WIDTH + x;
	vga_buffer[index] = vga_entry(c, color);
}


void term_init(void)
{
	term_row = term_col = 0;
	term_color = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_WHITE);

	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			vga_buffer[index] = vga_entry(0x0, term_color);
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

		case '\b': /* backspace */
			if (term_col != 0) {
				term_putentryat(0x0, term_color, term_col, term_row);
				term_col--;
			}
			break;

		default: {
			term_putentryat(c, term_color, term_col, term_row);
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
				vga_buffer[index] = vga_entry(0x0, term_color);
			}
		}
	}
}

void term_puts(char* data, size_t size)
{
	for (size_t i = 0; i < size; i++)
		term_putc(data[i]);
}
