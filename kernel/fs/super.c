#include <fs/devfs.h>
#include <kernel/kpanic.h>
#include <mm/slab.h>
#include <errno.h>

static mm_cache_t *sb_cache = NULL;
static mm_cache_t *sb_ops_cache = NULL;

static void init_sb_caches(void)
{
    if (!sb_cache) {
        if ((sb_cache = mmu_cache_create(sizeof(superblock_t), MM_NO_FLAG)) == NULL)
            kpanic("failed to allocate cache for superblocks!");
    }

    if (!sb_ops_cache) {
        if ((sb_ops_cache = mmu_cache_create(sizeof(super_ops_t), MM_NO_FLAG)) == NULL)
            kpanic("failed to allocate cache for superblock operations!");
    }
}

static superblock_t *alloc_generic_sb(void)
{
    superblock_t *sb = NULL;

    if (!sb_cache || !sb_ops_cache)
        init_sb_caches();

    if ((sb = mmu_cache_alloc_entry(sb_cache, MM_NO_FLAG)) == NULL) {
        kdebug("failed to allocate space for superblock!");
        return NULL;
    }

    if ((sb->s_ops = mmu_cache_alloc_entry(sb_ops_cache, MM_NO_FLAG)) == NULL) {
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
