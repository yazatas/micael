#include <kernel/kprint.h>
#include <kernel/tty.h> 

#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static void print_integer(uint32_t value, int width, int sign, bool zp)
{
	char c[64] = {0};
	int i = 0;

	while (value != 0) {
		c[i++] = (value % 10) + '0';
		value /= 10;
	}

	while (i < width && width != 0) {
		c[i++] = zp ? '0' : ' ';
	}

	if (sign == -1)
		c[i-1] = '-';

	while (--i >= 0) {
		term_putc(c[i]);
	}
}

static void va_kprint(const char *fmt, va_list args)
{
	int width = 0, zero_padding = 0;

	while (*fmt) {

		if (*fmt != '%') {
			term_putc(*fmt);
			fmt++;
			continue;
		}
		
		fmt++;
		switch (*fmt) {
			case '%':
				term_putc('%');
				fmt++;
				continue;

			case 'c':
				term_putc(va_arg(args, int));
				fmt++;
				continue;

			case 's': {
				char *tmp = va_arg(args, char*);
				term_puts(tmp);
				fmt++;
				continue;
			}

		}

		if (*fmt == '0') {
			zero_padding = 1;
			fmt++;
		}

		/* get padding width */
		while ((*fmt >= '0') && (*fmt <= '9')) {
			width = width * 10 + (*fmt - '0');
			fmt++;
		}

		switch (*fmt) {
			case 'd': {
				int32_t tmp = va_arg(args, int32_t);
				print_integer((tmp < 0) ? -tmp : tmp, width, -(tmp < 0), zero_padding);
				break;
			}

			case 'u': {
				uint32_t tmp = va_arg(args, uint32_t);
				print_integer(tmp, width, 0, zero_padding);
				break;
			}

			case 'x': {
				const uint8_t sym[16] = {'0', '1', '2', '3',
										 '4', '5', '6', '7',
										 '8', '9', 'a', 'b',
										 'c', 'd', 'e', 'f'};
				uint32_t tmp = va_arg(args, uint32_t);
				char c[64];
				int i = 0;

				while (tmp != 0) {
					c[i++] = sym[tmp & 0xf];
					tmp >>= 4;
				}

				while (i < width && width != 0) {
					c[i++] = zero_padding ? '0' : ' ';
				}

				while (--i >= 0) {
					term_putc(c[i]);
				}

				break;
			}
		}

		width = zero_padding = 0;
		fmt++;
	}
}

void kprint(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	va_kprint(fmt, args);
	va_end(args);
}
