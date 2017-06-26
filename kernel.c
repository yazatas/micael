#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
	#error "This code must be compiled with a cross-compiler!"
#elif !defined(__i386__)
	#error "This code must be compiled with an x86-elf compiler"
#endif


#define VGA_COLS 80
#define VGA_ROWS 25

volatile uint16_t *vga_buffer = (uint16_t*)0xb8000;

int term_col = 0;
int term_row = 0;
uint8_t term_color = 0xf0; /* black bg, white fg */

void term_init()
{
	for (int col = 0; col < VGA_COLS; ++col) {
		for (int row = 0; row < VGA_ROWS; ++row) {
			size_t index = (VGA_COLS * row) + col;

			/* first 8 bits: color, last 8 bits: ascii character */
			vga_buffer[index] = (term_color << 8) | ' ';
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
			size_t index = (VGA_COLS * term_row) + term_col;
			vga_buffer[index] = (term_color << 8) | c;
			term_col++;
			break;
		}
	}
	
	if (term_col >= VGA_COLS) {
		term_row++;
		term_col = 0;
	}

	if (term_row >= VGA_ROWS) {
		term_row = 0;
		term_col = 0;
	}
}

/* s must be null-terminated */
void term_puts(char *s)
{
	for (int i = 0; s[i] != '\0'; ++i) {
		term_putc(s[i]);
	}
}

void kernel_main(void)
{
	term_init();

	term_puts("Hello, world!\n");
}
