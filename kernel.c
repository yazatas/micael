#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
	#error "This code must be compiled with a cross-compiler!"
#elif !defined(__i386__)
	#error "This code must be compiled with an x86-elf compiler"
#endif


#define VGA_BLACK     0x0
#define VGA_BLUE      0x1
#define VGA_GREEN     0x2
#define VGA_CYAN      0x3
#define VGA_RED       0x4
#define VGA_MAGENTA   0x5
#define VGA_BROWN     0x6
#define VGA_GRAY      0x7
#define VGA_DARK_GRAY 0x8
#define VGA_BBLUE     0x9
#define VGA_BGREEN    0xa
#define VGA_BCYAN     0xb
#define VGA_BRED      0xc
#define VGA_BMAGENTA  0xd
#define VGA_YELLOW    0xe
#define VGA_WHITE     0xf

#define VGA_COLOR(bg, fg) (bg << 4 | fg)

#define VGA_COLS 80
#define VGA_ROWS 25

volatile uint16_t *vga_buffer = (uint16_t*)0xb8000;

int term_col = 0;
int term_row = 0;
uint8_t term_color = VGA_COLOR(VGA_BLACK, VGA_WHITE);

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
		for (int row = 1; row < VGA_ROWS; ++row) {
			for (int col = 0; col < VGA_COLS; ++col) {
				int prev = (VGA_COLS * row) + col - VGA_COLS;
				int cur  = (VGA_COLS * row) + col;
				vga_buffer[prev] = vga_buffer[cur];
			}
		}

		term_row = VGA_ROWS - 1;
		/* clear last line */
		if (c == '\n') {
			for (int col = 0; col < VGA_COLS; ++col) {
				int index = (VGA_COLS * term_row) + col;
				vga_buffer[index] = (term_color << 8) | ' ';
			}
		}
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
