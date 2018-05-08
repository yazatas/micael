ENTRY(_start)

SECTIONS
{
	. = 0xc0100000;
	__kernel_virtual_start = .;
	__kernel_physical_start = . - 0xc0000000;

	/* multiboot header and code */
	.text ALIGN(0x1000) : AT(ADDR(.text) - 0xc0000000)
	{
		*(.multiboot)
		__code_segment_start = . - 0xc0000000;
		*(.text)
		__code_segment_end = . - 0xc0000000;
	}

	/* Read-only data. */
	.rodata ALIGN(0x1000) : AT(ADDR(.rodata) - 0xc0000000)
	{
		*(.rodata)
	}

	/* Read-write data (initialized) */
	.data ALIGN(0x1000) : AT(ADDR(.data) - 0xc0000000)
	{
		__data_segment_start = . - 0xc0000000;
		*(.data)
		__data_segment_end = . - 0xc0000000;
	}

	/* Read-write data (uninitialized) and stack */
	.bss ALIGN(0x1000) : AT(ADDR(.bss) - 0xc0000000)
	{
		*(COMMON)
		*(.bss)
	}

	__kernel_virtual_end = .;
	__kernel_physical_end = . - 0xc0000000;
}
