#include <fs/fs.h>
#include <fs/multiboot.h>
#include <fs/super.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

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
#define MAX_FILES      5
#define MAX_DIRS       5

typedef struct disk_header {
    /* how much is the total size of initrd */
    uint32_t size;

    /* directory magic number (for verification) */
    uint32_t magic;

    /* how many directories initrd has */
    uint32_t num_dir;

    /* directory offsets (relative to disk's address) */
    uint32_t dir_offsets[MAX_DIRS];
} disk_header_t;

typedef struct dir_header {
    char name[NAME_MAXLEN];

    /* how many bytes directory takes in total:
     *  sizeof(dir_header_t) + num_files * sizeof(file_header_t) + file sizes */
    uint32_t size;

    /* how many files the directory has (1 <= num_files <= 5) */
    uint32_t num_files;

    uint32_t inode;
    uint32_t magic;

    uint32_t file_offsets[MAX_FILES];
} dir_header_t;

typedef struct file_header {
    char name[NAME_MAXLEN];

    /* file size in bytes (header size excluded) */
    uint32_t size;

    uint32_t inode;
    uint32_t magic;
} file_header_t;

typedef struct fs_private {
    disk_header_t *d_header;
    void *phys_start;
} fs_private_t;

typedef struct file_private {
    bool mapped;
    uint32_t offset; /* file offset from initrd start */
    void *data_ptr;  /* used to read/write from/to initrd */
} f_private_t;

static inode_t *initramfs_inode_alloc(superblock_t *sb);

static ssize_t initramfs_file_read(file_t *file, off_t offset, size_t count, void *buf)
{
    (void)file, (void)offset, (void)count, (void)buf;
    return -1;
}

static file_t *initramfs_file_open(dentry_t *dntr, int mode)
{
    if (!dntr) {
        errno = EINVAL;
        return NULL;
    }

    (void)dntr, (void)mode;
    return NULL;
}

static int initramfs_file_close(file_t *file)
{
    (void)file;
    return -1;
}

static off_t initramfs_file_seek(file_t *file, off_t offset)
{
    (void)file, (void)offset;
    return -1;
}

static inode_t *initramfs_inode_lookup(dentry_t *parent, char *name)
{
    if (!parent || !name) {
        errno = EINVAL;
        return NULL;
    }

    if ((parent->d_inode->i_flags & T_IFDIR) == 0) {
        errno = ENOTDIR;
        return NULL;
    }

    uint32_t i_ino    = 0;
    uint32_t i_size   = 0;
    uint32_t i_flags  = 0;
    void *i_private   = NULL;
    inode_t *inode    = NULL;
    dir_header_t *dh  = (dir_header_t *)parent->d_inode->i_private;

    for (size_t i = 0; i < dh->num_files; ++i) {
        dir_header_t *dir   = (dir_header_t  *)((uint8_t *)dh + dh->file_offsets[i]);
        file_header_t *file = (file_header_t *)((uint8_t *)dh + dh->file_offsets[i]);

        if (dir->magic == DIR_MAGIC) {
            if (strscmp(dir->name, name) == 0) {
                i_ino     = dir->inode;
                i_size    = dir->size;
                i_flags   = T_IFDIR;
                i_private = dir;
                goto found;
            }
        }

        if (file->magic == FILE_MAGIC) {
            if (strscmp(file->name, name) == 0) {
                i_ino     = file->inode;
                i_size    = file->size;
                i_flags   = T_IFREG;
                i_private = file;
                goto found;
            }
        }
    }

    errno = ENOENT;
    return NULL;

found:
    if ((inode = initramfs_inode_alloc(parent->d_inode->i_sb)) == NULL)
        return NULL;

    inode->i_ino     = i_ino;
    inode->i_size    = i_size;
    inode->i_flags   = i_flags;
    inode->i_private = i_private;

    return inode;
}

static int initramfs_inode_read(inode_t *ino)
{
    (void)ino;
    /* TODO: read size and inode number from disk */
    return 0;
}

static inode_t *initramfs_inode_alloc(superblock_t *sb)
{
    inode_t *ino = NULL;

    if ((ino = inode_alloc_empty(0)) == NULL)
        return NULL;

    ino->i_uid  = 0;
    ino->i_gid  = 0;
    ino->i_size = 0;
    ino->i_sb   = sb;

    ino->i_ops->lookup = initramfs_inode_lookup;
    ino->f_ops->read   = initramfs_file_read;
    ino->f_ops->open   = initramfs_file_open;
    ino->f_ops->close  = initramfs_file_close;
    ino->f_ops->seek   = initramfs_file_seek;

    ino->i_ops->create   = NULL; ino->i_ops->link     = NULL; ino->i_ops->unlink      = NULL;
    ino->i_ops->symlink  = NULL; ino->i_ops->mkdir    = NULL; ino->i_ops->rmdir       = NULL;
    ino->i_ops->mknod    = NULL; ino->i_ops->rename   = NULL; ino->i_ops->follow_link = NULL;
    ino->i_ops->put_link = NULL; ino->i_ops->truncate = NULL; ino->i_ops->permission  = NULL;
    ino->f_ops->write    = NULL;

    list_append(&ino->i_sb->s_ino, &ino->i_list);

    return ino;
}

static int initramfs_inode_destroy(inode_t *ino)
{
    return inode_dealloc(ino);
}

static int initramfs_init(superblock_t *sb, void *args)
{
    multiboot_module_t *mod;
    multiboot_info_t *mbi;
    disk_header_t *dh;

    /* first check did we actually get any modules */
    mbi = mmu_alloc_addr(1);
    mmu_map_page(args, mbi, MM_PRESENT | MM_READWRITE);

    if (mbi->mods_count == 0) {
        kdebug("trying to init initrd, module count 0!");
        mmu_free_addr(mbi, 1);
        return -1; /* TODO: proper errno */
    }

    /* kdebug("mbi->mods_count %u mbi->mods_addr 0x%x", mbi->mods_count, mbi->mods_addr); */

    /* then check that header was loaded correctly */
    mod = mmu_alloc_addr(1);
    mmu_map_page((char *)mbi->mods_addr, mod, MM_PRESENT | MM_READWRITE);
    mod = (multiboot_module_t *)((uint32_t)mod + 0x9c);

    /* kdebug("start 0x%x | end 0x%x | size %u", mod->mod_start, mod->mod_end, */
    /*         mod->mod_end - mod->mod_start); */

    dh = mmu_alloc_addr(1);
    mmu_map_page((char *)mod->mod_start, dh, MM_PRESENT | MM_READWRITE);

    if (dh->magic != HEADER_MAGIC) {
        kdebug("invalid header magic: 0x%x", dh->magic);
        return -EINVAL;
    }

    sb->s_root          = dentry_alloc_orphan("/", T_IFDIR);
    sb->s_root->d_inode = initramfs_inode_alloc(sb);

    sb->s_private = kmalloc(sizeof(fs_private_t));
    ((fs_private_t *)sb->s_private)->d_header   = dh;
    ((fs_private_t *)sb->s_private)->phys_start = (char *)mod->mod_start;

    sb->s_root->d_inode->i_private = (char *)((uint32_t)dh + sizeof(disk_header_t));
    sb->s_root->d_inode->i_flags   = T_IFDIR;
    sb->s_root->d_inode->i_ino     = 1;

    mmu_free_addr(mbi, 1);
    mmu_free_addr(mod, 1);

    return 0;
}

superblock_t *initramfs_get_sb(fs_type_t *type, char *dev, int flags, void *data)
{
    (void)dev, (void)flags;

    superblock_t *sb = NULL;

    if ((sb        = kmalloc(sizeof(superblock_t))) == NULL ||
        (sb->s_ops = kmalloc(sizeof(super_ops_t)))  == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }

    sb->s_blocksize = 0;
    sb->s_count     = 1;
    sb->s_dirty     = 0;
    sb->s_private   = NULL;
    sb->s_root      = NULL;
    sb->s_type      = type;
    sb->s_magic     = HEADER_MAGIC;

    sb->s_ops->destroy_inode = initramfs_inode_destroy;
    sb->s_ops->alloc_inode   = initramfs_inode_alloc;
    sb->s_ops->read_inode    = initramfs_inode_read;

    sb->s_ops->delete_inode  = NULL;
    sb->s_ops->write_super   = NULL;
    sb->s_ops->dirty_inode   = NULL;
    sb->s_ops->write_inode   = NULL;
    sb->s_ops->put_inode     = NULL;
    sb->s_ops->put_super     = NULL;
    sb->s_ops->sync_fs       = NULL;

    list_init(&sb->s_ino);
    list_init(&sb->s_ino_dirty);
    list_init(&sb->s_instances);

    initramfs_init(sb, data);

    return sb;
}

int initramfs_kill_sb(superblock_t *sb)
{
    if (!sb)
        return -EINVAL;

    if (sb->s_count > 1)
        return -EBUSY;

    kfree(sb->s_ops);
    kfree(sb);

    return 0;
}
