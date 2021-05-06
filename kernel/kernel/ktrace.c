#include <fs/elf.h>
#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <kernel/util.h>

#define CALL_STACK_MAX_DEPTH 8

static uint8_t *__sym    = NULL;
static uint8_t *__str    = NULL;
static size_t __sym_size = 0;
static size_t __str_size = 0;

int ktrace_register(uint8_t *sym, size_t sym_size, uint8_t *str, size_t str_size)
{
    __sym      = sym;
    __str      = str;
    __sym_size = sym_size;
    __str_size = str_size;

    /* TODO: build new array with only functions? */
}

static int __trace(uint64_t rbp)
{
    Elf64_Sym *iter = __sym;

    for (int i = 0; i < __sym_size; ) {
        iter = __sym + i;

        if (iter->st_value <= rbp && rbp <= iter->st_value + iter->st_size) {
            if (ELF64_ST_TYPE(iter->st_info) == STT_FUNC) {
                kprint("--> 0x%x %s\n", rbp, &__str[iter->st_name]);

                if (!kstrcmp(&__str[iter->st_name], "init_bsp") ||
                    !kstrcmp(&__str[iter->st_name], "init_ap") ||
                    !kstrcmp(&__str[iter->st_name], "init_task_func"))
                {
                    return 1;
                }
                return 0;
            } else {
                kprint("--> 0x%x\n", rbp);
            }
        }

        i += sizeof(Elf64_Sym);
    }
}

void ktrace(void)
{
    uint64_t *rbp = 0;

    asm volatile ("mov %%rbp, %0": "=g"(rbp));

    for (int i = 0; i < CALL_STACK_MAX_DEPTH; ++i) {
        if (__trace(rbp[1]))
            return;
        rbp = rbp[0];
    }
}
