#include <fs/vfs.h>
#include <fs/initrd.h>
#include <mm/kheap.h>
#include <mm/vmm.h>
#include <fs/multiboot.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>

#include <string.h>

uint32_t initrd_lookup(const char *path)
{
    return 0;
}

uint32_t initrd_read(file_t *file, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    (void)file, (void)offset, (void)size, (void)buffer;
    return 0;
}

void initrd_open(const char *path, uint8_t mode)
{
    (void)path, (void)mode;
}

fs_t *initrd_create(void)
{
    fs_t *fs = kmalloc(sizeof(fs_t));

    return fs;
}

void initrd_destroy(fs_t *fs)
{
    kfree(fs);
}

disk_header_t *initrd_init(multiboot_info_t *mbinfo)
{
    file_header_t *fh;
    multiboot_info_t *mbi;
    multiboot_module_t *mod;

    mbi = __vmm_map_page(mbinfo, NULL);

    if (mbi->mods_count == 0) {
        kpanic("trying to init initrd, module count 0!");
        __builtin_unreachable();
    }

    kdebug("mbi->mods_count %u mbi->mods_addr 0x%x", mbi->mods_count, mbi->mods_addr);

    mod = (multiboot_module_t *)((uint32_t)__vmm_map_page((void *)mbi->mods_addr, NULL) + 0x9c);

    kdebug("mod start end size 0x%x 0x%x %u", mod->mod_start,
            mod->mod_end, mod->mod_end - mod->mod_start);

    disk_header_t *dh = initrd_init(mbinfo);
    void **memories = NULL;
    size_t file_sizes[2] = { 0 };

    kdebug("file count: %u | disk size: %u | magic: 0x%x", 
            dh->file_count, dh->disk_size, dh->magic);

    fh = (file_header_t *)((uint8_t*)dh + sizeof(disk_header_t));

    uint8_t *mem_ptr   = NULL;
    uint8_t *ptr       = NULL;
    uint8_t *f_start_p = NULL;
    uint8_t *f_start_v = NULL;
    
    memories = kmalloc(sizeof(void *) * dh->file_count);

    for (size_t i = 0; i < dh->file_count; ++i) {

        kdebug("file start virt 0x%x", f_start_v);
        kdebug("file start phys 0x%x\n", f_start_p);
        kdebug("name '%s' | size %u | magic 0x%x", fh->file_name, fh->file_size, fh->magic);

        ptr = memories[i] = kmalloc(fh->file_size);
        file_sizes[i] = fh->file_size;

        memcpy(ptr, (uint8_t *)&fh->magic + 4, fh->file_size);

        fh = (file_header_t *)((uint8_t *)fh + sizeof(file_header_t) + fh->file_size);
    }

    kdebug("DONED FINALLY");
    kdebug("tring to jump to user mode");


    return (disk_header_t *)__vmm_map_page((void *)mod->mod_start, NULL);
}
