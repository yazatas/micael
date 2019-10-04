#include <fs/elf.h>
#include <fs/file.h>
#include <kernel/kpanic.h>
#include <kernel/util.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <sched/sched.h>
#include <stdbool.h>

#include <arch/x86_64/mm/mmu.h>

#ifdef __x86_64__
#define USER_STACK_START 0xc0000000
#else
#define KSTART 768
#define USER_STACK_START ((((KSTART - 1) << 22) | (1023 << 12)) | 0xfee)
#endif

static bool __check_common_elf_header(Elf32_Ehdr *header)
{
    if (header->e_ident[EI_MAG0] != ELFMAG0 ||
        header->e_ident[EI_MAG1] != ELFMAG1 ||
        header->e_ident[EI_MAG2] != ELFMAG2 ||
        header->e_ident[EI_MAG3] != ELFMAG3)
    {
        kdebug("Invalid ELF header magic number!");
        hex_dump(header->e_ident, SELFMAG);
        return false;
    }

    if (header->e_type != ET_EXEC) {
        kdebug("Object file is not executable: %u", header->e_type);
        return false;
    }

    return true;
}

static bool __loader_32(void *addr, int argc, char **argv)
{
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)addr;
    Elf32_Phdr *phdr = (Elf32_Phdr *)((uint8_t *)ehdr + ehdr->e_phoff);

    if (!__check_common_elf_header(ehdr)) {
        kdebug("Invalid ELF header!");
        return false;
    }

    if (ehdr->e_machine != EM_386) {
        kdebug("Object file ISA is not 80386: %u", ehdr->e_machine);
        return false;
    }

    if (phdr->p_type == PT_LOAD) {
        int mm_flags = MM_PRESENT | MM_USER;

        if ((phdr->p_flags & PF_W) == 0)
            mm_flags |= MM_READONLY;

        unsigned long v_start = ROUND_DOWN(phdr->p_vaddr, PAGE_SIZE);
        size_t nleft          = phdr->p_filesz;
        void *fptr            = (uint8_t *)phdr + phdr->p_offset + ehdr->e_phentsize;
        size_t nwritten       = 0;

        while (nwritten < nleft) {
            unsigned long page = mmu_page_alloc(MM_ZONE_NORMAL);
            mmu_map_page(page, v_start, mm_flags);

            size_t read_size = MIN(PAGE_SIZE, nleft);
            kmemcpy((void *)(unsigned long)ehdr->e_entry, fptr, read_size);

            nwritten += read_size;
            v_start   = v_start + read_size;
        }
    }

    /* allocate user stack and copy argc + argv there */
    uint32_t mm_flags = MM_PRESENT | MM_READWRITE | MM_USER;
    mmu_map_page(mmu_page_alloc(MM_ZONE_NORMAL), USER_STACK_START - PAGE_SIZE, mm_flags);

    /* TODO: where is argv mapped? */
    /* TODO: add argc + argv to stack */
    (void)argc, (void)argv;

    sched_enter_userland(
        (void *)(unsigned long)ehdr->e_entry,
        (void *)(unsigned long)USER_STACK_START - 4
    );
}

static bool __loader_64(void *addr, int argc, char **argv)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)addr;
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uint8_t *)ehdr + ehdr->e_phoff);

    if (!__check_common_elf_header((Elf32_Ehdr *)ehdr)) {
        kdebug("Invalid ELF header!");
        return false;
    }

    if (ehdr->e_machine != EM_X86_64) {
        kdebug("Object file ISA is not amd64: %u", ehdr->e_machine);
        return false;
    }

    if (phdr->p_type == PT_LOAD) {
        int mm_flags = MM_PRESENT | MM_USER;

        if ((phdr->p_flags & PF_W) == 0)
            mm_flags |= MM_READONLY;

        unsigned long v_start = ROUND_DOWN(phdr->p_vaddr, PAGE_SIZE);
        size_t nleft          = phdr->p_filesz;
        void *fptr            = (uint8_t *)phdr + phdr->p_offset + ehdr->e_phentsize;
        size_t nwritten       = 0;

        while (nwritten < nleft) {
            unsigned long page = mmu_page_alloc(MM_ZONE_NORMAL);
            mmu_map_page(page, v_start, mm_flags);

            size_t read_size = MIN(PAGE_SIZE, nleft);
            kmemcpy((void *)(unsigned long)ehdr->e_entry, fptr, read_size);

            nwritten += read_size;
            v_start   = v_start + read_size;
        }
    }

    /* allocate user stack and copy argc + argv there */
    uint32_t mm_flags = MM_PRESENT | MM_READWRITE | MM_USER;
    mmu_map_page(mmu_page_alloc(MM_ZONE_NORMAL), USER_STACK_START - PAGE_SIZE, mm_flags);

    /* TODO: where is argv mapped? */
    /* TODO: add argc + argv to stack */
    (void)argc, (void)argv;

    sched_enter_userland(
        (void *)(unsigned long)ehdr->e_entry,
        (void *)(unsigned long)USER_STACK_START - 8
    );
}

/* This is the ELF loader stub, it will just check whether the the
 * file in question is 32 or 64-bit and call the appropriate handler */
bool binfmt_elf_loader(file_t *file, int argc, char **argv)
{
    (void)argc, (void)argv;

    kdebug("in binfmt_elf_loader...");

    unsigned long mem = mmu_block_alloc(MM_ZONE_NORMAL, 1);
    size_t fsize      = file->f_dentry->d_inode->i_size;
    size_t pages      = (fsize / PAGE_SIZE) + 1;
    void *addr        = (void *)mem;

    if (file_read(file, 0, fsize, addr) < 0) {
        kdebug("Failed to read data from file!");
        return false;
    }

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)addr;

    if (ehdr->e_ident[EI_CLASS] == ELFCLASS32)
        return __loader_32(addr, argc, argv);
    else if (ehdr->e_ident[EI_CLASS] == ELFCLASS64)
        return __loader_64(addr, argc, argv);

    return false;
}
