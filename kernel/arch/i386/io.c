#include <kernel/io.h>

void outb(uint32_t addr, uint8_t v)
{
	asm volatile("outb %%al, %%dx" :: "d" (addr), "a" (v));
}

void outw(uint32_t addr, uint16_t v)
{
	asm volatile("outw %%ax, %%dx", :: "d" (addr), "a" (v));
}

void outl(uint32_t addr, uint32_t v)
{
	asm volatile("outl %%eax, %%dx", :: "d" (addr), "a" (v));
}

uint8_t inb(uint32_t addr)
{
	uint8_t v;
	asm volatile("inb %%dx, %%al" : "=a" (v) : "d" (addr));
	return v;
}

uint16_t inw(uint32_t addr)
{
	uint16_t v;
	asm volatile("inw %%dx, %%ax" : "=a" (v) : "d" (addr));
	return v;
}

uint32_t inl(uint32_t addr)
{
	uint32_t v;
	asm volatile("inl %%dx, %%eax" : "=a" (v) : "d" (addr));
	return v;
}
