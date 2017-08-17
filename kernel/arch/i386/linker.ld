ENTRY(_start)

SECTIONS
{
	. = 0xc0100000;
	__kernel_start = .;

	/* multiboot header and code */
	.text ALIGN(0x1000) : AT(ADDR(.text) - 0xc0000000)
	{
		*(.multiboot)
		*(.text)
	}

	/* Read-only data. */
	.rodata ALIGN(0x1000) : AT(ADDR(.rodata) - 0xc0000000)
	{
		*(.rodata)
	}

	/* Read-write data (initialized) */
	.data ALIGN(0x1000) : AT(ADDR(.data) - 0xc0000000)
	{
		*(.data)
	}

	/* Read-write data (uninitialized) and stack */
	.bss ALIGN(0x1000) : AT(ADDR(.bss) - 0xc0000000)
	{
		*(COMMON)
		*(.bss)
	}

	__kernel_end = .;
}
