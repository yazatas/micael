#include <fs/char.h>
#include <fs/devfs.h>
#include <fs/fs.h>
#include <kernel/util.h>
#include <lib/bitmap.h>
#include <mm/slab.h>
#include <errno.h>

static mm_cache_t *cdev_cache  = NULL;
static bitmap_t *bm_devnums = NULL;

/* FIXME: this is incorrect, each device major group
 * should have it's own set of minor numbers */
static int cdev_alloc_devnum(void)
{
    int minor = bm_find_first_unset(bm_devnums, 0, bm_devnums->len - 1);

    if (minor < 0)
        kdebug("failed to allocate device minor number!");

    return minor;
}

int cdev_init(void)
{
    dentry_t *dntr = NULL;
    path_t *path   = NULL;
    int ret        = 0;

    if ((cdev_cache = mmu_cache_create(sizeof(cdev_t), MM_NO_FLAG)) == NULL)
        return -ENOMEM;

    if ((bm_devnums = bm_alloc_bitmap(256)) == NULL)
        return -ENOMEM;

    path = vfs_path_lookup("/dev", LOOKUP_OPEN);

    if (path->p_status == LOOKUP_STAT_ENOENT) {
        kdebug("failed to find /dev, unable to initialize cdev subsystem!");
        ret = -ENOENT;
        goto end;
    }

    if ((dntr = dentry_alloc_orphan("char", T_IFDIR)) == NULL) {
        ret = -errno;
        goto end;
    }

    if (!path->p_dentry->d_inode) {
        ret = -EINVAL;
        goto end;
    }

    if (!path->p_dentry->d_inode->i_iops->mkdir) {
        ret = -ENOSYS;
        goto end;
    }

    if ((ret = path->p_dentry->d_inode->i_iops->mkdir(path->p_dentry, dntr, 0)) < 0)
        kdebug("failed to create /dev/char!");

end:
    vfs_path_release(path);
    return ret;
}

cdev_t *cdev_alloc(char *name, file_ops_t *ops, int major)
{
    if (name == NULL || ops == NULL) {
        errno = EINVAL;
        return NULL;
    }

    int minor   = -1;
    cdev_t *dev = NULL;

    if ((minor = cdev_alloc_devnum()) < 0)
        goto error;

    if ((dev = mmu_cache_alloc_entry(cdev_cache, MM_NO_FLAG)) == NULL)
        goto error_deventry;

    dev->c_dev  = MAKE_DEV(major, minor);
    dev->c_ops  = ops;
    dev->c_name = name;

    return dev;

error_deventry:
    bm_unset_bit(bm_devnums, minor);

error:
    kdebug("failed to allocate character driver");
    return NULL;
}

void cdev_dealloc(cdev_t *dev)
{
    if (!dev)
        return;

    /* try to unregister the device, it might be in use so ignore the error */
    (void)devfs_unregister_cdev(dev);

    if (bm_unset_bit(bm_devnums, DEV_MINOR(dev->c_dev)) < 0) {
        kdebug("failed to unset bit");
    }

    mmu_cache_free_entry(cdev_cache, dev);
}
