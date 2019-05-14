OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
	. = 4M;

	.loader.text :
	{
		arch/x86_64/boot.o (.multiboot)
		arch/x86_64/boot.o (.text)
	}	

	.loader.rodata :
	{
		arch/x86_64/boot.o (.rodata)
	}

	.loader.data :
	{
		arch/x86_64/boot.o (.data)
	}

	.loader.bss :
	{
		arch/x86_64/boot.o (COMMON)
		arch/x86_64/boot.o (.bss)
	}

	/* TODO: this looks ugly, is merging these statements possible?? */
	. = ALIGN(0x1000);
	_loader_end = .;

	. += 0xffffffff80000000;

	.text : AT (_loader_end)
	{
		*(.multiboot)
		*(.text)
	}

	. = ALIGN(0x1000);

	.rodata : AT ((LOADADDR(.text) + SIZEOF(.text) + 0xFFF) & 0xFFFFFFFFFFFFF000)
	{
		*(.rodata*)
	}

	. = ALIGN(0x1000);

	.data : AT ((LOADADDR(.rodata) + SIZEOF(.rodata) + 0xFFF) & 0xFFFFFFFFFFFFF000)
	{
		*(.data)
	}

	. = ALIGN(0x1000);

	.bss : AT ((LOADADDR(.data) + SIZEOF(.data) + 0xFFF) & 0xFFFFFFFFFFFFF000)
	{
		*(.bss)
		*(COMMON)
	}

	_kernel_physical_end = (LOADADDR(.bss) + SIZEOF(.bss) + 0xFFF) & 0xFFFFFFFFFFFFF000;
}
