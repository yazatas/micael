#include <fs/elf.h>
#include <fs/fs.h>
#include <kernel/kpanic.h>
#include <mm/mmu.h>
#include <sched/sched.h>
#include <string.h>

#define USER_STACK_START ((((KSTART - 1) << 22) | (1023 << 12)) | 0xfee)

static bool elf_check_header(Elf32_Ehdr *header)
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

    if (header->e_ident[EI_CLASS] != ELFCLASS32) {
        kdebug("Object file is not 32-bit!");
        return false;
    }

    if (header->e_type != ET_EXEC) {
        kdebug("Object file is not executable: %u", header->e_type);
        return false;
    }

    if (header->e_machine != EM_386) {
        kdebug("Object file ISA is not 80386: %u", header->e_machine);
        return false;
    }

    return true;
}

bool binfmt_elf_loader(file_t *file, int argc, char **argv)
{
    (void)argc, (void)argv;

    kdebug("in binfmt_elf_loader...");

    size_t fsize = file->f_dentry->d_inode->i_size;
    size_t pages = (fsize / PAGE_SIZE) + 1;
    void *addr   = NULL, *ptr = NULL;

    if ((addr = ptr = mmu_alloc_addr(pages)) == NULL) {
        kdebug("Failed to allocate address space for file!");
        return false;
    }

    for (size_t i = 0; i < pages; ++i) {
        page_t page = mmu_alloc_page();
        mmu_map_page((void *)page, ptr, MM_PRESENT | MM_READWRITE);
        ptr = (uint8_t *)ptr + PAGE_SIZE;
    }

    if (vfs_read(file, 0, fsize, addr) < 0) {
        kdebug("Failed to read data from file!");
        return false;
    }

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)addr;

    if (!elf_check_header(ehdr)) {
        kdebug("Invalid ELF header!");
        return false;
    }

    /* kprint("\tentry point:                  0x%08x\n" */
    /*        "\tprogram header table:         0x%08x\n" */
    /*        "\tsection header table:         0x%08x\n" */
    /*        "\tsizeof(program header entry):   %8u\n" */
    /*        "\t# of program header entries:    %8u\n" */
    /*        "\tsizeof(section header entry):   %8u\n" */
    /*        "\t# of program header entries:    %8u\n", */
    /*     ehdr->e_entry, ehdr->e_phoff, ehdr->e_shoff, */
    /*     ehdr->e_phentsize, ehdr->e_phnum, */ 
    /*     ehdr->e_shentsize, ehdr->e_shnum); */

    Elf32_Phdr *phdr = (Elf32_Phdr *)((uint8_t *)ehdr + ehdr->e_phoff);

    if (phdr->p_type == PT_LOAD) {
        /* kprint("\tp_offset:   %8u\n", phdr->p_offset); */
        /* kprint("\tp_vaddr:  0x%08x\n", phdr->p_vaddr); */
        /* kprint("\tp_paddr:  0x%08x\n", phdr->p_paddr); */
        /* kprint("\tp_filesz:   %8u\n", phdr->p_filesz); */
        /* kprint("\tp_memsz:    %8u\n", phdr->p_memsz); */
        /* kprint("\tp_flags:    %8u\n", phdr->p_flags); */
        /* kprint("\tp_align:    %8u\n", phdr->p_align); */
        kdebug("loadable segment found!");

        uint32_t mm_flags = MM_PRESENT | MM_USER;

        if ((phdr->p_flags & PF_W) == 0)
            mm_flags |= MM_READONLY;

        uint32_t v_start = ROUND_DOWN(phdr->p_vaddr, PAGE_SIZE);
        size_t nleft     = phdr->p_filesz;
        size_t nwritten  = 0;
        void *fptr       = (uint8_t *)phdr + phdr->p_offset + ehdr->e_phentsize;

        while (nwritten < nleft) {
            page_t page = mmu_alloc_page();
            mmu_map_page((void *)page, (void *)v_start, mm_flags);

            size_t read_size = MIN(PAGE_SIZE, nleft);

            memcpy((void *)ehdr->e_entry, fptr, read_size);

            nwritten += read_size;
            v_start   = v_start + read_size;
        }
    }

    /* create stack */
    uint32_t mm_flags = MM_PRESENT | MM_READWRITE | MM_USER;
    mmu_map_page((void *)mmu_alloc_page(), (void *)USER_STACK_START, mm_flags);

    /* TODO: add argc + argv to stack */

    vmm_flush_TLB();

    sched_enter_userland((void *)ehdr->e_entry, (void *)USER_STACK_START);
}
