#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include <fs/fs.h>
#include <fs/initrd.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <fs/dcache.h>
#include <fs/multiboot.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>

/* This implementation of the initrd is very naive.
 * It only supports the most necessary features,
 * below is the current spec:
 *
 * 1) Current maximum depth of directories is two. That is,
 *    only root and one additional directory is possible.
 *
 * 2) Root directory cannot contain any files, only directories
 *
 * 3) Directory can contain up to 5 files (root directory can contain
 *    5 directories and these directories can contain up to 5 files)
 */

#define HEADER_MAGIC  0x13371338
#define FILE_MAGIC    0xdeadbeef
#define DIR_MAGIC     0xcafebabe

#define NAME_MAXLEN   64
#define MAX_FILES     5
#define MAX_DIRS      5

typedef struct disk_header {
    uint8_t  file_count;
    uint32_t disk_size;
    uint32_t magic;
} disk_header_t;

typedef struct dir_header {
    char dir_name[NAME_MAXLEN];
    uint32_t item_count;
    uint32_t bytes_len;
    uint32_t inode;
    uint32_t magic;
} dir_header_t;

typedef struct file_header {
    char file_name[NAME_MAXLEN];
    uint32_t file_size;
    uint32_t inode;
    uint32_t magic;
} file_header_t;

typedef struct fs_private {
    disk_header_t *d_header;
    void *phys_start;
} fs_private_t;

typedef struct file_private {
    off_t offset;
    uint32_t phys_start;
} f_private_t;

#define GET_HEADER(fs)            (((fs_private_t *)fs->private)->d_header)
#define GET_PHYS_START(fs)        (((fs_private_t *)fs->private)->phys_start)
#define SET_HEADER(fs, dh)        (((fs_private_t *)fs->private)->d_header = (dh))
#define SET_PHYS_START(fs, start) (((fs_private_t *)fs->private)->phys_start = (start))

static inode_t *initrd_alloc_inode(fs_t *fs);
static int initrd_seek_file(file_t *file, off_t offset);

static ssize_t initrd_read_file(file_t *file, off_t offset, size_t len, void *buf)
{
    int ret = -1;
    file_header_t *fh = file->private;

    if (len > fh->file_size)
        return -E2BIG;

    if (offset != 0) {
        if ((ret = initrd_seek_file(file, offset)) < 0)
            return ret;
    }

    if (fh->file_size < len + file->offset)
        return -E2BIG;

    void *ptr = (uint8_t *)fh + sizeof(file_header_t) + file->offset;

    memcpy(buf, ptr, len);
    file->offset += len;

    return (ssize_t)len;
}

/* TODO: make sure that this is the last refrence to this file */
static void initrd_close_file(file_t *file)
{
    if (--file->refcount == 0) {
        /* TODO: closing should be ok */
        kdebug("file refcount > 1 (%u), can't close it", file->refcount);
        errno = EBUSY;
        return;
    }

    kfree(file->f_dentry);
    kfree(file);
}

/* make sure that resulting offset (file->offset + offset)
 * is not less than 0 and that the new offset is not larger than FILE_SIZE */
static int initrd_seek_file(file_t *file, off_t offset)
{
    const uint32_t FILE_SIZE = file->f_dentry->d_inode->i_size;

    if (file->offset < -offset || file->offset + offset > (off_t)FILE_SIZE)
        return -ESPIPE;

    file->offset = file->offset + offset;
    return 0;
}

static file_t *initrd_open_file(dentry_t *dntr, uint8_t mode)
{
    /* initrd is read-only */
    if ((mode & VFS_WRITE) != 0) {
        errno = EINVAL;
        return NULL;
    }

    /* only files are readable (for now) */
    if ((dntr->d_inode->flags & VFS_TYPE_FILE) == 0) {
        errno = EINVAL;
        return NULL;
    }

    file_t *fp = kmalloc(sizeof(file_t));

    if (!fp)
        return NULL;

    if ((fp->f_ops  = kmalloc(sizeof(struct file_ops))) == NULL) {
        kfree(fp);
        return NULL;
    }

    fp->f_ops->read  = initrd_read_file;
    fp->f_ops->open  = initrd_open_file;
    fp->f_ops->close = initrd_close_file;
    fp->f_ops->seek  = initrd_seek_file;
    fp->f_ops->write = NULL;

    fp->refcount = 1;
    fp->offset   = 0;
    fp->f_dentry = dntr;
    fp->private  = dntr->private;

    return fp;
}

static void initrd_free_inode(fs_t *fs, inode_t *ino)
{
    (void)fs;
    kfree(ino);
}

/* TODO: check for resource leaks */
static inode_t *initrd_lookup_inode(fs_t *fs, void *param)
{
    (void)fs, (void)param;
    char *to_free, *str, *tok;
    uint32_t phys_offset = 0;

    kdebug("nothing found, returning NULL");

    return NULL;
}

static dentry_t *initrd_lookup_dentry(fs_t *fs, dentry_t *haystack, const char *needle)
{
    /* first check if the haystack's children hashmap has been created already. If it has been,
     * jump straight to searching the needle. Otherwise build the hashmap for before searching */
    if (hm_get_size(haystack->children) > 0)
        goto search;

    dentry_t *dntr      = NULL;
    dir_header_t *dir   = (dir_header_t *)haystack->private;
    file_header_t *file = (file_header_t *)((uint8_t *)dir + sizeof(dir_header_t));

    for (size_t i = 0; i < dir->item_count; ++i) {
        dntr = dentry_alloc(file->file_name);
        dntr->d_inode = initrd_alloc_inode(fs);

        dntr->d_inode->i_ino  = file->inode;
        dntr->d_inode->i_size = file->file_size;
        dntr->d_inode->flags  = VFS_TYPE_FILE;
        dntr->private         = file;

        hm_insert(haystack->children, file->file_name, dntr);

        file = (file_header_t *)((uint8_t *)file + file->file_size + sizeof(file_header_t));
    }

search:
    return hm_get(haystack->children, (void *)needle);
}

static inode_t *initrd_alloc_inode(fs_t *fs)
{
    (void)fs;

    inode_t *ino = kmalloc(sizeof(inode_t));

    ino->private = NULL;
    ino->i_ino   = ino->i_size = 0;
    ino->i_uid   = ino->i_gid  = 0;
    ino->flags   = ino->mask   = 0;

    ino->i_ops = kmalloc(sizeof(struct inode_ops));
    ino->f_ops = kmalloc(sizeof(struct file_ops));

    ino->i_ops->lookup_inode  = initrd_lookup_inode;
    ino->i_ops->lookup_dentry = initrd_lookup_dentry;
    ino->i_ops->alloc_inode   = initrd_alloc_inode;
    ino->i_ops->free_inode    = initrd_free_inode;

    ino->f_ops->read  = initrd_read_file;
    ino->f_ops->open  = initrd_open_file;
    ino->f_ops->close = initrd_close_file;
    ino->f_ops->seek  = initrd_seek_file;

    ino->f_ops->write        = NULL;
    ino->i_ops->write_inode  = NULL;
    ino->i_ops->read_inode   = NULL;

    return ino;
}

fs_t *initrd_create(void *args)
{
    multiboot_module_t *mod;
    multiboot_info_t *mbi;
    disk_header_t *dh;

    /* first check did we actually get any modules */
    mbi = vmm_alloc_addr(1);
    vmm_map_page(args, mbi, MM_PRESENT | MM_READWRITE);

    if (mbi->mods_count == 0) {
        kdebug("trying to init initrd, module count 0!");
        vmm_free_addr(mbi, 1);
        return NULL;
    }

    kdebug("mbi->mods_count %u mbi->mods_addr 0x%x", mbi->mods_count, mbi->mods_addr);

    /* then check that header was loaded correctly */
    mod = vmm_alloc_addr(1);
    vmm_map_page((void *)mbi->mods_addr, mod, MM_PRESENT | MM_READWRITE);
    mod = (multiboot_module_t *)((uint32_t)mod + 0x9c);

    kdebug("start 0x%x | end 0x%x | size %u", mod->mod_start, mod->mod_end,
            mod->mod_end - mod->mod_start);

    dh = vmm_alloc_addr(1);
    vmm_map_page((void *)mod->mod_start, dh, MM_PRESENT | MM_READWRITE);

    if (dh->magic != HEADER_MAGIC) {
        kdebug("invalid header magic: 0x%x", dh->magic);
        return NULL;
    }

    fs_t *fs    = kmalloc(sizeof(fs_t));
    fs->root    = dentry_alloc("initrd");
    fs->private = kmalloc(sizeof(fs_private_t));

    fs->root->d_flags  = 0;
    fs->root->d_parent = NULL;
    fs->root->d_inode  = initrd_alloc_inode(fs);

    fs->root->d_inode->i_ino = 1; /* root inode */

    SET_HEADER(fs, dh);
    SET_PHYS_START(fs, (void *)mod->mod_start);

    kdebug("file count: %u | disk size: %u", 
            GET_HEADER(fs)->file_count, GET_HEADER(fs)->disk_size);

    /* initialize root's children */
    fs->root->private  = (void *)((uint32_t)dh + sizeof(disk_header_t));
    dir_header_t *iter = (dir_header_t *)((uint8_t *)fs->root->private + sizeof(dir_header_t));

    for (size_t i = 0; i < ((dir_header_t *)fs->root->private)->item_count; ++i) {
        dentry_t *dntr = dentry_alloc(iter->dir_name);
        dntr->d_inode  = initrd_alloc_inode(fs);

        dntr->d_inode->i_ino  = iter->inode;
        dntr->d_inode->i_size = iter->bytes_len;
        dntr->private         = (void *)iter;

        iter = (dir_header_t *)((uint8_t *)iter + sizeof(dir_header_t) + iter->bytes_len);
    }

    vmm_free_addr(mbi, 1);
    vmm_free_addr(mod, 1);

    return fs;
}

void initrd_destroy(fs_t *fs)
{
    fs_private_t *fs_priv = fs->private;

    vmm_free_addr(fs_priv->d_header, 1);
    kfree(fs->root->d_inode);
    kfree(fs->root);
    kfree(fs->private);
    kfree(fs);
}
