#include <fs/vfs.h>
#include <fs/initrd.h>
#include <mm/kheap.h>
#include <mm/vmm.h>
#include <fs/multiboot.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>

#include <string.h>

#define HEADER_MAGIC  0x13371338
#define FILE_MAGIC    0xdeadbeef
#define FNAME_MAXLEN  64

struct disk_header {
    uint8_t  file_count;
    uint32_t disk_size;
    uint32_t magic;
} *disk_header;

typedef struct file_header {
    char file_name[FNAME_MAXLEN];
    uint32_t file_size;
    uint32_t magic;
} file_header_t;

static uint32_t initrd_lookup(const char *path)
{
    return 0;
}

static uint32_t initrd_read(file_t *file, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    (void)file, (void)offset, (void)size, (void)buffer;
    return 0;
}

static void initrd_open(const char *path, uint8_t mode)
{
    (void)path, (void)mode;
}

static fs_t *initrd_create(void)
{
    fs_t *fs = kmalloc(sizeof(fs_t));

    return fs;
}

static void initrd_destroy(fs_t *fs)
{
    kfree(fs);
}

vfs_status_t initrd_init(multiboot_info_t *mbinfo)
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

    disk_header = __vmm_map_page((void *)mod->mod_start, NULL);

    kdebug("file count: %u | disk size: %u | magic: 0x%x", 
            disk_header->file_count, disk_header->disk_size, disk_header->magic);

    fh = (file_header_t *)((uint8_t*)disk_header + sizeof(struct disk_header));

    for (size_t i = 0; i < disk_header->file_count; ++i) {
        kdebug("name '%s' | size %u | magic 0x%x", fh->file_name, fh->file_size, fh->magic);
        fh = (file_header_t *)((uint8_t *)fh + sizeof(file_header_t) + fh->file_size);
    }
}
