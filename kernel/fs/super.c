#include <fs/devfs.h>
#include <fs/super.h>
#include <kernel/kpanic.h>
#include <mm/cache.h>
#include <errno.h>

static cache_t *sb_cache = NULL;
static cache_t *sb_ops_cache = NULL;

static void init_sb_caches(void)
{
    if (!sb_cache) {
        if ((sb_cache = cache_create(sizeof(superblock_t), C_NOFLAGS)) == NULL)
            kpanic("failed to allocate cache for superblocks!");
    }

    if (!sb_ops_cache) {
        if ((sb_ops_cache = cache_create(sizeof(super_ops_t), C_NOFLAGS)) == NULL)
            kpanic("failed to allocate cache for superblock operations!");
    }
}

static superblock_t *alloc_generic_sb(void)
{
    superblock_t *sb = NULL;

    if (!sb_cache || !sb_ops_cache)
        init_sb_caches();

    if ((sb = cache_alloc_entry(sb_cache, C_NOFLAGS)) == NULL) {
        kdebug("failed to allocate space for superblock!");
        return NULL;
    }

    if ((sb->s_ops = cache_alloc_entry(sb_ops_cache, C_NOFLAGS)) == NULL) {
        kdebug("failed to allocate space for superblock operations!");
        return NULL;
    }

    return sb;
}

superblock_t *
super_get_sb_pseudo(fs_type_t *type, char *dev, int flags,
                    void *data, int (*fill_super)(superblock_t *))
{
    (void)dev, (void)flags, (void)data, (void)fill_super;

    if (!type) {
        errno = EINVAL;
        return NULL;
    }

    return NULL;
}

superblock_t *
super_get_sb_single(fs_type_t *type, char *dev, int flags,
                    void *data, int (*fill_super)(superblock_t *))
{
    (void)dev, (void)data, (void)type;

    superblock_t *sb = alloc_generic_sb();

    if (!sb || fill_super(sb) < 0)
        return NULL;

    sb->s_flags = flags;
    sb->s_bdev  = NULL;
    sb->s_dev   = MAKE_DEV(0, devfs_alloc_minor(0));

    return sb;
}

superblock_t *
super_get_sb_nodev(fs_type_t *type, char *dev, int flags,
                   void *data, int (*fill_super)(superblock_t *))
{
    (void)dev, (void)flags, (void)data, (void)fill_super;

    if (!type) {
        errno = EINVAL;
        return NULL;
    }

    return NULL;
}

superblock_t *
super_get_sb_bdev(fs_type_t *type, char *dev, int flags,
                  void *data, int (*fill_super)(superblock_t *))
{
    (void)dev, (void)flags, (void)data, (void)fill_super;

    if (!type) {
        errno = EINVAL;
        return NULL;
    }

    return NULL;
}

#if 0
    The most important operations performed by get_sb_bdev() are the following:

    1. Invokes open_bdev_excl() to open the block device having device file name dev_
    name (see the section “Character Device Drivers” in Chapter 13).

    2. Invokes sget() to search the list of superblock objects of the filesystem (type->
    fs_supers, see the earlier section “Filesystem Type Registration”). If a super-
    block relative to the block device is already present, the function returns its
    address. Otherwise, it allocates and initializes a new superblock object, inserts it
    into the filesystem list and in the global list of superblocks, and returns its
    address.

    3. If the superblock is not new (it was not allocated in the previous step, because
    the filesystem is already mounted), it jumps to step 6.

    4. Copies the value of the flags parameter into the s_flags field of the superblock
    and sets the s_id, s_old_blocksize, and s_blocksize fields with the proper val-
    ues for the block device.

    5. Invokes the filesystem-dependent function passed as last argument to get_sb_
    bdev() to access the superblock information on disk and fill the other fields of
    the new superblock object.

    6. Returns the address of the new superblock object.
#endif
