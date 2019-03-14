#include <fs/binfmt.h>
#include <fs/block.h>
#include <fs/char.h>
#include <fs/dentry.h>
#include <fs/devfs.h>
#include <fs/fs.h>
#include <fs/initramfs.h>
#include <lib/hashmap.h>
#include <lib/list.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/util.h>
#include <mm/cache.h>
#include <mm/heap.h>
#include <sched/sched.h>
#include <errno.h>
#include <stdbool.h>

#define NUM_FS_TYPES 2
#define NUM_FS 1

static cache_t *path_cache;

static list_head_t mountpoints;
static list_head_t superblocks;

static mount_t *root_fs;
static hashmap_t *fs_types;

/* array of "static" file systems (known at compile time)
 * that kernel needs in order to work. Keeping them here
 * makes the initialization code much cleaner.
 * (same thing with the mountpoint array below) */
static fs_type_t fs_arr[NUM_FS_TYPES] = {
    {
        .fs_name = "devfs",
        .get_sb  = devfs_get_sb,
        .kill_sb = devfs_kill_sb
    },
    {
        .fs_name = "initramfs",
        .get_sb  = initramfs_get_sb,
        .kill_sb = initramfs_kill_sb
    }
};

/* TODO: explain this struct */
static struct {
    char *type;
    char *target;
    dentry_t *mount;
} file_systems[NUM_FS] = {
    {
        .type   = "devfs",
        .target = "dev",
        .mount  = NULL
    },
};

static mount_t *alloc_empty_mount(void)
{
    mount_t *mnt = kmalloc(sizeof(mount_t));
    
    if (!mnt)
        return NULL;

    mnt->mnt_sb         = NULL;
    mnt->mnt_root       = NULL;
    mnt->mnt_devname    = NULL;
    mnt->mnt_type       = NULL;
    mnt->mnt_mount = NULL;

    list_init(&mnt->mnt_list);

    return mnt;
}

/* TODO: explain why this is static instead of public */
static int vfs_mount_pseudo(char *target, char *type, dentry_t *mountpoint)
{
    mount_t *mnt  = NULL;
    fs_type_t *fs = NULL;

    if (!target || !type || !mountpoint)
        return -EINVAL;

    if ((fs = hm_get(fs_types, (char *)type)) == NULL) {
        kdebug("filesystem type %s has not been registered!", type);
        return -ENOSYS;
    }

    if ((mnt = alloc_empty_mount()) == NULL) {
        kdebug("failed to allocate mountpoint for %s", type);
        return -ENOMEM;
    }

    mnt->mnt_sb      = fs->get_sb(fs, NULL, 0, NULL);
    mnt->mnt_type    = type;
    mnt->mnt_devname = NULL;
    mnt->mnt_mount   = mountpoint;

    mountpoint->d_count++;
    mnt->mnt_root = NULL; /* TODO: how to get this from the filesystem??? */

    list_append(&mountpoints, &mnt->mnt_list);

    return 0;
}

void vfs_init(void)
{
    fs_types   = hm_alloc_hashmap(16, HM_KEY_TYPE_STR);
    path_cache = cache_create(sizeof(path_t), C_NOFLAGS);

    list_init(&mountpoints);
    list_init(&superblocks);

    dentry_init();
    inode_init();
    file_init();
    cdev_init();
    bdev_init();

    binfmt_init();
    binfmt_add_loader(binfmt_elf_loader);

    /* register all known filesystems that might be needed */
    for (int i = 0; i < NUM_FS_TYPES; ++i) {
        if (vfs_register_fs(&fs_arr[i]) < 0)
            kdebug("failed to register %s", fs_arr[i].fs_name);
    }

    /* create empty root (no mounted filesystem [yet]) */
    root_fs            = alloc_empty_mount();
    root_fs->mnt_mount = dentry_alloc_orphan("/", T_IFDIR);
    root_fs->mnt_type  = "rootfs";

    list_append(&mountpoints, &root_fs->mnt_list);

    /* mount the pseudo filesystems, we must use vfs_mount_pseudo()
     * to mount these special file systems which skips most of the error 
     * checks and path look ups.
     *
     * This is done because scheduler hasn't been initialized and
     * vfs_path_lookup() uses "current" to determine the path. */
    for (int i = 0; i < NUM_FS; ++i) {
        file_systems[i].mount = dentry_alloc(root_fs->mnt_mount,
                                             file_systems[i].target, T_IFDIR);

        if (file_systems[i].mount == NULL) {
            kdebug("failed to dentry for /%s", file_systems[i].target);
            continue;
        }

        if (vfs_mount_pseudo(file_systems[i].target, file_systems[i].type,
                             file_systems[i].mount) < 0)
        {
            kdebug("failed to mount %s to /%s!", file_systems[i].type, file_systems[i].target);
        }
    }
}

int vfs_install_rootfs(char *type, void *data)
{
    fs_type_t *fs = hm_get(fs_types, type);

    if (fs == NULL)
        return -ENOTSUP;

    if (root_fs->mnt_sb != NULL)
        return -EBUSY;

    if ((root_fs->mnt_sb = initramfs_get_sb(fs, NULL, 0, data)) == NULL)
        return -errno;

    root_fs->mnt_root    = root_fs->mnt_sb->s_root;
    root_fs->mnt_type    = "initramfs";
    root_fs->mnt_devname = NULL;

    list_append(&superblocks, &root_fs->mnt_sb->s_list);

    return 0;
}

int vfs_register_fs(fs_type_t *fs)
{
    if (!fs || !fs->get_sb || !fs->kill_sb)
        return -EINVAL;

    if (hm_get(fs_types, (char *)fs->fs_name) != NULL)
        return -EEXIST;

    return hm_insert(fs_types, (char *)fs->fs_name, fs);
}

int vfs_unregister_fs(char *type)
{
    if (!type || hm_get(fs_types, type) == NULL)
        return -EINVAL;

    FOREACH(mountpoints, m) {
        mount_t *mnt = container_of(m, mount_t, mnt_list);

        if (strscmp(type, mnt->mnt_type) == 0)
            return -EBUSY;
    }

    return hm_remove(fs_types, type);
}

int vfs_mount(char *source, char *target,
              char *type,   uint32_t flags)
{
    (void)flags;

    fs_type_t *fs   = NULL;
    path_t    *path = NULL;
    mount_t   *mnt  = NULL;
    dentry_t  *src  = NULL,
              *dst  = NULL;

    if (!target || !type)
        return -EINVAL;

    if ((fs = hm_get(fs_types, (char *)type)) == NULL) {
        kdebug("file system '%s' has not been registered!", type);
        return -ENOTSUP;
    }

    /* devfs, sysfs and procfs can be mounted only once */
    if (strscmp(type, "devfs")  == 0 ||
        strscmp(type, "sysfs")  == 0 ||
        strscmp(type, "procfs") == 0)
    {
        if (source != NULL) {
            kdebug("%s doesn't take source!", type);
            return -EINVAL;
        }

        /* go through the mounted filesystems and check if filesystem
         * of "type" has already been mounted -> return error */
        FOREACH(mountpoints, m) {
            mnt = container_of(m, mount_t, mnt_list);
            
            if (strscmp(mnt->mnt_type, type) == 0) {
                kdebug("%s has already been mounted to %s", type, target);
                return -EEXIST;
            }
        }    

        goto check_dest;
    }

    /* if source is NULL, the only possible (valid) filesystem is tmpfs */
    if (source == NULL) {
        if (strscmp(type, "tmpfs") != 0) {
            kdebug("source can't be NULL for %s", type);
            return -EINVAL;
        }
    } else {
        if ((path = vfs_path_lookup(source, 0))->p_dentry == NULL) {
            kdebug("'%s' does not exist!", source);
            return -ENOENT;
        }
    }

check_dest:
    /* now check that "target" exist and it doesn't have a filesystem installed on it */
    if ((path = vfs_path_lookup(target, 0))->p_dentry == NULL) {
        kdebug("mountpoint %s does not exist!", target);
        return -ENOENT;
    }

    /* TODO: there must a better way to do this */
    FOREACH(mountpoints, m) {
        mnt = container_of(m, mount_t, mnt_list);
        
        if (strscmp(mnt->mnt_type, type) == 0) {
            kdebug("%s has already been mounted to %s", type, target);
            return -EEXIST;
        }
    }

    if ((mnt = alloc_empty_mount()) == NULL) {
        kdebug("failed to allocate mountpoint for %s", type);
        return -ENOMEM;
    }

    mnt->mnt_sb      = fs->get_sb(fs, source, 0, NULL);
    mnt->mnt_type    = type;
    mnt->mnt_devname = source;
    mnt->mnt_mount   = dst;

    dst->d_count++;
    mnt->mnt_root = NULL; /* TODO: how to get this from the filesystem??? */

    list_append(&mountpoints, &mnt->mnt_list);

    return 0;
}

static char *vfs_extract_child(char **path)
{
    char *start = *path,
         *ptr   = *path;
    size_t len  = strlen(*path);
    size_t i    = 0;

    for (i = 0; i < len && ptr[i] != '/' && ptr[i] != '\0'; ++i) {
        (*path)++;
    }

    /* there are more dentries left, add null byte to mark next start */
    if (i != len) {
        (*path)++;
        ptr[i] = '\0';
    } else {
        /* we're at the end
         * set path to NULL so vfs_walk_path() knows to end the path lookup */
        *path = NULL;
    }

    if (len == 0)
        return NULL;

    return start;
}

static dentry_t *vfs_walk_path(dentry_t *parent, char *path, int flags)
{
    if (!parent || !path) {
        errno = EINVAL;
        return NULL;
    }

    dentry_t *dntr = NULL;
    inode_t *ino   = NULL;
    char *next     = vfs_extract_child(&path);

    if (path == NULL) {
        if (flags & LOOKUP_PARENT)
            return parent;

        /* return dentry if found from parent's hashmap  */
        if ((dntr = hm_get(parent->d_children, next)) != NULL)
            return dntr;

        if ((ino = inode_lookup(parent, next)) == NULL)
            return NULL;

        if ((dntr = dentry_alloc_ino(parent, next, ino, ino->i_flags)) == NULL)
            return NULL;

        return dntr;
    }

    if (!parent->d_children) {
        kdebug("parent ('%s') doesn't have a valid children hashmap!", parent->d_name);
        errno = EINVAL;
        return NULL;
    }

    /* check if parent has this dentry already cached, continue immediately if so */
    if ((dntr = hm_get(parent->d_children, next)) != NULL)
        return vfs_walk_path(dntr, path, flags);

    /* not found from the parent hashmap, do filesystem-specific search */
    if ((ino = inode_lookup(parent, next)) == NULL)
        return NULL;

    if ((dntr = dentry_alloc_ino(parent, next, ino, ino->i_flags)) == NULL)
        return NULL;

    return vfs_walk_path(dntr, path, flags);
}

path_t *vfs_path_lookup(char *path, int flags)
{
    if (!path) {
        errno = EINVAL;
        return NULL;
    }

    path_t *retpath = NULL;
    task_t *current = NULL;
    dentry_t *start = NULL,
             *dntr  = NULL;
    char *_path_ptr = NULL,
         *_path     = NULL;

    _path_ptr = _path = strdup(path);
    retpath   = kmalloc(sizeof(path_t));

    if (_path[0] == '/') {
        _path = _path + 1; /* skip '/' */
        start = root_fs->mnt_root;
    } else {
        if ((current = sched_get_current()) == NULL) {
            kdebug("scheduler has not been started, but relative path was given!");
            errno = ENOTSUP;
            goto end;
        }

        if (!current->fs_ctx) {
            kdebug("running task doesn't have file system context!");
            errno = EINVAL;
            goto end;
        }

        start = current->fs_ctx->pwd;
    }

    if (start == NULL) {
        kdebug("bootstrap dentry is NULL!");
        errno = EINVAL;
        goto end;
    }

    if ((retpath->p_dentry = vfs_walk_path(start, _path, flags)) != NULL)
        retpath->p_dentry->d_count++;

    retpath->p_flags = flags;

end:
    kfree(_path_ptr);
    return retpath;
}

int vfs_path_release(path_t *path)
{
    if (!path || !path->p_dentry)
        return -EINVAL;

    if (--path->p_dentry->d_count == 1)
        dentry_dealloc(path->p_dentry);

    kfree(path);

    return 0;
}
