#ifndef __X86_64_ASM_H__
#define __X86_64_ASM_H__

#include <kernel/compiler.h>

typedef struct task task_t;

void gdt_flush(void *ptr);
void idt_flush(void *ptr);
void tss_flush(int index);

#endif /* __X86_64_ASM_H____X86_64_ASM_H__ */
