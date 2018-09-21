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

    return (disk_header_t *)__vmm_map_page((void *)mod->mod_start, NULL);
}
