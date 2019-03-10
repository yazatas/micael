#include <fs/dentry.h>
#include <fs/file.h>
#include <fs/inode.h>
#include <kernel/kpanic.h>
#include <mm/cache.h>
#include <errno.h>

static cache_t *inode_cache     = NULL;
static cache_t *inode_ops_cache = NULL;
static cache_t *file_ops_cache  = NULL;

int inode_init(void)
{
    if ((inode_cache = cache_create(sizeof(inode_t), C_NOFLAGS)) == NULL)
        kpanic("failed to initialize slab cache for inodes!");

    if ((inode_ops_cache = cache_create(sizeof(inode_ops_t), C_NOFLAGS)) == NULL)
        kpanic("failed to initialize slab cache for inode ops!");

    if ((file_ops_cache = cache_create(sizeof(file_ops_t), C_NOFLAGS)) == NULL)
        kpanic("failed to initialize slab cache for file ops!");

    return 0;
}

inode_t *inode_alloc_empty(uint32_t flags)
{
    inode_t *ino = NULL;

    if (((ino        = cache_alloc_entry(inode_cache,     C_NOFLAGS)) == NULL) ||
        ((ino->f_ops = cache_alloc_entry(file_ops_cache,  C_NOFLAGS)) == NULL) ||
        ((ino->i_ops = cache_alloc_entry(inode_ops_cache, C_NOFLAGS)) == NULL))
    {
        if (ino) {
            if (ino->i_ops)
                cache_dealloc_entry(inode_ops_cache, ino->i_ops, C_NOFLAGS);
            cache_dealloc_entry(inode_cache, ino, C_NOFLAGS);
        }

        errno = ENOMEM;
        return NULL;
    }

    /* TODO: get these from sched? */
    ino->i_uid   = -1;
    ino->i_gid   = -1;

    ino->i_ino   = -1;
    ino->i_size  = -1;
    ino->i_mask  = 0;
    ino->i_count = 1;
    ino->i_flags = flags;

    return ino;
}

int inode_dealloc(inode_t *ino)
{
    if (!ino)
        return -EINVAL;

    if (ino->i_count > 1)
        return -EBUSY;

    cache_dealloc_entry(inode_ops_cache, ino->i_ops, C_NOFLAGS);
    cache_dealloc_entry(file_ops_cache,  ino->f_ops, C_NOFLAGS);
    cache_dealloc_entry(inode_cache,     ino,        C_NOFLAGS);

    return 0;
}

inode_t *inode_lookup(dentry_t *dntr, char *name)
{
    if (!dntr || !dntr->d_inode || !dntr->d_inode->i_ops || !name) {
        errno = EINVAL;
        return NULL;
    }

    if (!dntr->d_inode->i_ops->lookup) {
        errno = ENOSYS;
        return NULL;
    }

    return dntr->d_inode->i_ops->lookup(dntr, name);
}
