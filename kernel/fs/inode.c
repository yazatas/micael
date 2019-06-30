#include <fs/dentry.h>
#include <fs/file.h>
#include <fs/inode.h>
#include <kernel/kpanic.h>
#include <mm/slab.h>
#include <errno.h>

static mm_cache_t *inode_cache     = NULL;
static mm_cache_t *inode_ops_cache = NULL;
static mm_cache_t *file_ops_cache  = NULL;

int inode_init(void)
{
    if ((inode_cache = mmu_cache_create(sizeof(inode_t), MM_NO_FLAG)) == NULL)
        kpanic("failed to initialize slab cache for inodes!");

    if ((inode_ops_cache = mmu_cache_create(sizeof(inode_ops_t), MM_NO_FLAG)) == NULL)
        kpanic("failed to initialize slab cache for inode ops!");

    if ((file_ops_cache = mmu_cache_create(sizeof(file_ops_t), MM_NO_FLAG)) == NULL)
        kpanic("failed to initialize slab cache for file ops!");

    return 0;
}

inode_t *inode_generic_alloc(uint32_t flags)
{
    inode_t *ino = NULL;

    if (((ino         = mmu_cache_alloc_entry(inode_cache,     MM_NO_FLAG)) == NULL) ||
        ((ino->i_fops = mmu_cache_alloc_entry(file_ops_cache,  MM_NO_FLAG)) == NULL) ||
        ((ino->i_iops = mmu_cache_alloc_entry(inode_ops_cache, MM_NO_FLAG)) == NULL))
    {
        if (ino) {
            if (ino->i_iops)
                mmu_cache_free_entry(inode_ops_cache, ino->i_iops);
            mmu_cache_free_entry(inode_cache, ino);
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

int inode_generic_dealloc(inode_t *ino)
{
    if (!ino)
        return -EINVAL;

    if (ino->i_count > 1)
        return -EBUSY;

    (void)mmu_cache_free_entry(inode_ops_cache, ino->i_iops);
    (void)mmu_cache_free_entry(file_ops_cache,  ino->i_fops);
    (void)mmu_cache_free_entry(inode_cache,     ino);

    return 0;
}

inode_t *inode_lookup(dentry_t *dntr, char *name)
{
    if (!dntr || !dntr->d_inode || !dntr->d_inode->i_iops || !name) {
        errno = EINVAL;
        return NULL;
    }

    if (!dntr->d_inode->i_iops->lookup) {
        errno = ENOSYS;
        return NULL;
    }

    return dntr->d_inode->i_iops->lookup(dntr, name);
}