#include <fs/fs.h>
#include <fs/multiboot.h>
#include <fs/super.h>
#include <kernel/kpanic.h>
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

typedef struct inode_private_t {
    void *pstart;
    void *vstart;
    int num_mapped;
} i_private_t;

typedef struct file_private {
    bool mapped;
    int num_mapped;
    void *pstart; /* points to physical memory */
    void *vstart; /* points to allocated virtual memory (this must be released) */
} f_private_t;

#define GET_FILE_PRIVATE(f) ((f_private_t *)(f->f_private))
#define GET_INO_PRIVATE(i)  ((i_private_t *)(i->i_private))

static inode_t *initramfs_inode_lookup(dentry_t *parent, char *name);
static inode_t *initramfs_inode_alloc(superblock_t *sb);
static int      initramfs_inode_destroy(inode_t *ino);

static int     initramfs_file_close(file_t *file);
static int     initramfs_file_seek(file_t *file, off_t offset);
static file_t *initramfs_file_open(dentry_t *dntr, int mode);


static ssize_t initramfs_file_read(file_t *file, off_t offset, size_t count, void *buf)
{
    if (!file || !buf)
        return -EINVAL;

    /* allocate address space for file and map the bytes there */
    if (GET_FILE_PRIVATE(file)->mapped == false) {
#if 0
        size_t range = (file->f_dentry->d_inode->i_size / 4096) + 2;
        void *vaddr  = mmu_alloc_addr(range);
        void *paddr  = GET_FILE_PRIVATE(file)->pstart;
        size_t off   = (uint32_t)paddr - ROUND_DOWN(((uint32_t)paddr), PAGE_SIZE);

        mmu_map_range(paddr, vaddr, range, MM_PRESENT | MM_READONLY);

        GET_FILE_PRIVATE(file)->mapped     = true;
        GET_FILE_PRIVATE(file)->num_mapped = range;
        GET_FILE_PRIVATE(file)->vstart     = (char *)((uint32_t)vaddr + off);
#endif
    }

    int ret    = initramfs_file_seek(file, offset);
    char *addr = ((char *)GET_FILE_PRIVATE(file)->vstart) + sizeof(file_header_t);

    if (ret < 0)
        return ret;

    if (((off_t)count + file->f_pos) > file->f_dentry->d_inode->i_size)
        return -E2BIG;

    kmemcpy(buf, addr + file->f_pos, count);
    return count;
}

static file_t *initramfs_file_open(dentry_t *dntr, int mode)
{
    if (!dntr || !dntr->d_inode) {
        errno = EINVAL;
        return NULL;
    }

	/* initramfs is a read-only filesystem */
    if (mode != O_RDONLY) {
        errno = ENOTSUP;
        return NULL;
    }

    if ((dntr->d_inode->i_flags & T_IFREG) == 0) {
        errno = EINVAL;
        return NULL;
    }

    file_t *file = file_generic_alloc();

    if (file == NULL)
        return NULL;

    file->f_pos     = 0;
    file->f_count   = 1;
    file->f_mode    = mode;
    file->f_dentry  = dntr;
    file->f_ops     = dntr->d_inode->i_fops;
    dntr->d_count++;

    if ((file->f_private = kmalloc(sizeof(f_private_t))) == NULL) {
        (void)file_generic_dealloc(file);
        return NULL;
    }

    GET_FILE_PRIVATE(file)->mapped = false;
    GET_FILE_PRIVATE(file)->vstart = NULL;
    GET_FILE_PRIVATE(file)->pstart = GET_INO_PRIVATE(dntr->d_inode)->pstart;

    return file;
}

static int initramfs_file_close(file_t *file)
{
    if (!file)
        return -EINVAL;

    if (file->f_count == 1 &&
        GET_FILE_PRIVATE(file)->mapped &&
        GET_FILE_PRIVATE(file)->vstart != NULL)
    {
        /* mmu_free_addr(GET_FILE_PRIVATE(file)->vstart, GET_FILE_PRIVATE(file)->num_mapped); */
    }

    kfree(file->f_private);
    return file_generic_dealloc(file);
}

static int initramfs_file_seek(file_t *file, off_t offset)
{
    return file_generic_seek(file, offset);
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
    uint32_t i_offset = 0;
    inode_t *inode    = NULL;

    i_private_t *ip  = ((i_private_t *)parent->d_inode->i_private);
    dir_header_t *dh = (dir_header_t *)ip->vstart;
    
    for (size_t i = 0; i < dh->num_files; ++i) {
        i_offset            = dh->file_offsets[i];
        dir_header_t *dir   = (dir_header_t  *)((char *)dh + i_offset);
        file_header_t *file = (file_header_t *)((char *)dh + i_offset);

        if (dir->magic == DIR_MAGIC) {
            if (kstrcmp_s(dir->name, name) == 0) {
                i_ino   = dir->inode;
                i_size  = dir->size;
                i_flags = T_IFDIR;
                goto found;
            }
        }

        if (file->magic == FILE_MAGIC) {
            if (kstrcmp_s(file->name, name) == 0) {
                i_ino   = file->inode;
                i_size  = file->size;
                i_flags = T_IFREG;
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

    /* if the item found was directory, check whether its children fit into 
     * the memory area allocated by the parent.
     *
     * If they do, make i_private->vstart point to parent->...->i_private->vstart + offset.
     * If, however, the children occupies more memory than currently has been mapped,
     * we must map the whole directory to memory and make i_private->vstart
     * point to this new area
     *
     * if the item was file, no need allocate any memory/address space yet */
    inode->i_private      = kmalloc(sizeof(i_private_t));
    uint32_t parent_start = (unsigned long)ip->pstart;
    uint32_t parent_size  = parent->d_inode->i_size;
    uint32_t boundary     = ROUND_DOWN(parent_start, PAGE_SIZE);
    uint32_t off_start    = parent_start + i_offset - boundary;

    /* if the offset from page boundary of the initrd + directory size is more than PAGE_SIZE,
     * it means that this directory overlaps two pages and we must allocate space for two pages */
    if ((i_flags & T_IFDIR) && (off_start + i_size) > PAGE_SIZE) {
#if 0
        uint32_t *vstart      = mmu_alloc_addr(2);
        size_t off_from_start = parent_start + i_offset - boundary;
        mmu_map_range((void *)boundary, vstart, 2, MM_PRESENT | MM_READONLY);

        GET_INO_PRIVATE(inode)->pstart = (void *)((uint8_t *)ip->pstart + i_offset);
        GET_INO_PRIVATE(inode)->vstart = (void *)((uint8_t *)vstart + off_start);
#endif
    } else {
        if (i_flags & T_IFDIR)
            GET_INO_PRIVATE(inode)->vstart = (char *)ip->vstart + i_offset;
        else
            GET_INO_PRIVATE(inode)->vstart = NULL;

        GET_INO_PRIVATE(inode)->pstart = (char *)ip->pstart + i_offset;
    }

    return inode;
}

static inode_t *initramfs_inode_alloc(superblock_t *sb)
{
    inode_t *ino = NULL;

    if ((ino = inode_generic_alloc(0)) == NULL)
        return NULL;

    ino->i_uid  = 0;
    ino->i_gid  = 0;
    ino->i_size = 0;
    ino->i_sb   = sb;

    ino->i_iops->lookup = initramfs_inode_lookup;
    ino->i_fops->read   = initramfs_file_read;
    ino->i_fops->open   = initramfs_file_open;
    ino->i_fops->close  = initramfs_file_close;
    ino->i_fops->seek   = initramfs_file_seek;

    ino->i_iops->create   = NULL; ino->i_iops->link     = NULL; ino->i_iops->unlink      = NULL;
    ino->i_iops->symlink  = NULL; ino->i_iops->mkdir    = NULL; ino->i_iops->rmdir       = NULL;
    ino->i_iops->mknod    = NULL; ino->i_iops->rename   = NULL; ino->i_iops->follow_link = NULL;
    ino->i_iops->put_link = NULL; ino->i_iops->truncate = NULL; ino->i_iops->permission  = NULL;
    ino->i_fops->write    = NULL;

    list_append(&ino->i_sb->s_ino, &ino->i_list);

    return ino;
}

static int initramfs_inode_destroy(inode_t *ino)
{
    /* TODO: free i_private */
    return inode_generic_dealloc(ino);
}

static int initramfs_init(superblock_t *sb, void *args)
{
    multiboot_module_t *mod;
    multiboot_info_t *mbi;
    disk_header_t *dh;

#if 0
    /* first check did we actually get any modules */
    /* mbi = mmu_alloc_addr(1); */
    mmu_map_page(args, mbi, MM_PRESENT | MM_READWRITE);

    if (mbi->mods_count == 0) {
        kdebug("trying to init initrd, module count 0!");
        /* mmu_free_addr(mbi, 1); */
        return -ENXIO;
    }

    /* kdebug("mbi->mods_count %u mbi->mods_addr 0x%x", mbi->mods_count, mbi->mods_addr); */

    /* then check that header was loaded correctly */
    /* mod = mmu_alloc_addr(1); */
    mmu_map_page((char *)mbi->mods_addr, mod, MM_PRESENT | MM_READWRITE);
    mod = (multiboot_module_t *)((uint32_t)mod + 0x9c);

    /* kdebug("start 0x%x | end 0x%x | size %u", mod->mod_start, mod->mod_end, */
    /*         mod->mod_end - mod->mod_start); */

    /* dh = mmu_alloc_addr(1); */
    mmu_map_page((char *)mod->mod_start, dh, MM_PRESENT | MM_READWRITE);

    if (dh->magic != HEADER_MAGIC) {
        kdebug("invalid header magic: 0x%x", dh->magic);
        return -EINVAL;
    }
#endif

    sb->s_root          = dentry_alloc_orphan("/", T_IFDIR);
    sb->s_root->d_inode = initramfs_inode_alloc(sb);

    sb->s_private = kmalloc(sizeof(fs_private_t));
    ((fs_private_t *)sb->s_private)->d_header   = dh;
    ((fs_private_t *)sb->s_private)->phys_start = (char *)(unsigned long)mod->mod_start;

    sb->s_root->d_inode->i_ino     = 1;
    sb->s_root->d_inode->i_flags   = T_IFDIR;

    sb->s_root->d_inode->i_private = kmalloc(sizeof(i_private_t));
    
    /* physical start address of '/' */
    ((i_private_t *)sb->s_root->d_inode->i_private)->pstart =
        ((char *)(unsigned long)mod->mod_start) + sizeof(disk_header_t);

    /* virtual (mapped) start of '/' */
    ((i_private_t *)sb->s_root->d_inode->i_private)->vstart =
        ((char *)dh)            + sizeof(disk_header_t);

    /* mmu_free_addr(mbi, 1); */
    /* mmu_free_addr(mod, 1); */

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

    sb->s_ops->read_inode    = NULL;
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
    kfree(sb->s_private);
    /* TODO: release allocated addreses and pages */
    kfree(sb);

    return 0;
}
