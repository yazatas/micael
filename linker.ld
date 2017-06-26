ENTRY(start)

SECTIONS
{
	/* start at 1M, below that exists x86 related stuff we don't want to overwrite */
	. = 1M;

	/* make all sections 4K aligned, this'll help later on when paging is added */

	/* multiboot header */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
	}

	/* executable code */
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.text)
	}

	/* read-only data */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	/* read-write data (initialized) */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	/* read-write data (uninitialized) */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
}
