#include <fs/binfmt.h>
#include <fs/dcache.h>
#include <fs/initrd.h>
#include <fs/fs.h>
#include <kernel/kprint.h>
#include <mm/heap.h>

#include <errno.h>
#include <string.h>

#define NUM_FS      1
#define NUM_LOADERS 1

static mount_t root_fs;

static binfmt_loader_t loaders[NUM_LOADERS] = {
    binfmt_elf_loader
};

static fs_type_t fs_types[NUM_FS] = {
    {
        .fs_dev_id  = 0x1338,
        .fs_name    = "initrd",
        .fs_create  = initrd_create,
        .fs_destroy = initrd_destroy,
    },
};

static fs_type_t *vfs_find_fs(const char *fs_name)
{
    for (int i = 0; i < NUM_FS; ++i) {
        if (strcmp(fs_name, fs_types[i].fs_name) == 0) {
            kdebug("fs found: '%s' %u", fs_types[i].fs_name, fs_types[i].fs_dev_id);
            return &fs_types[i];
        }
    }

    return NULL;
}

/* vfs_get_mountpoint expects full path f.ex /usr/bin/cat */
static char *vfs_get_mountpoint(const char *path)
{
    const char *start = path + 1;
    const char *end   = strchr(start, '/');
    const size_t len  = (uint32_t)(end - start);

    char *res = kmalloc(len + 1);

    memcpy(res, start, (uint32_t)(end - start));
    res[len] = '\0';

    return res;
}

void vfs_init(void *args)
{
    (void)args;

    kdebug("initializing VFS...");

    root_fs.mountpoint = kmalloc(sizeof(dentry_t));
    root_fs.fs         = NULL;
    root_fs.dev_id     = 0;

    dentry_t *tmp  = root_fs.mountpoint;
    tmp->d_flags   = 0;
    tmp->d_parent  = NULL; /* FIXME: ??? */
    tmp->d_inode   = NULL; /* FIXME: ??? */
    tmp->d_name[0] = '/';
    tmp->d_name[1] = '\0';

    list_init_null(&root_fs.mountpoints);

    dcache_init();
    binfmt_init();

    for (int i = 0; i < NUM_LOADERS; ++i) {
        binfmt_add_loader(loaders[i]);
    }
}

fs_t *vfs_register_fs(const char *fs_name, const char *mnt, void *arg)
{
    kdebug("registering file system '%s', mount point '%s'", fs_name, mnt);

    mount_t *mount;
    list_head_t *iter;
    fs_type_t *fs_type;

    FOREACH(&root_fs.mountpoints, iter) {
        mount_t *tmp = container_of(iter, mount_t, mountpoints);
        kdebug("'%s'", tmp->mountpoint->d_name);

        /* mountpoint already exists, check if it points to valid file system */
        if (strcmp(tmp->mountpoint->d_name, mnt + 1) == 0) {
            if (tmp->fs != NULL) {
                kdebug("'%s' already exists and has file system installed on it!");
                errno = EEXIST;
                return NULL;
            }
        }
    }

    if ((fs_type = vfs_find_fs(fs_name)) == NULL)
        return NULL;

    mount = kmalloc(sizeof(mount_t));

    mount->dev_id = fs_type->fs_dev_id;
    mount->fs     = fs_type->fs_create(arg);
    mount->fs->fs = fs_type;

    char *tmp = strdup(mnt);
    
    mount->mountpoint = dentry_alloc(mnt + 1); /* discard '/' */
    mount->mountpoint->d_parent = root_fs.mountpoint;
    mount->mountpoint->d_inode  = mount->fs->root->d_inode;

    kdebug("allocated dentry name %s", mount->mountpoint->d_name);

    list_append(&root_fs.mountpoints, &mount->mountpoints);

    return mount->fs;
}

int vfs_unregister_fs(fs_t *fs)
{
    if (!fs)
        return -EINVAL;

    if (fs->fs->fs_destroy)
        fs->fs->fs_destroy(fs);

    list_head_t *iter;
    FOREACH(&root_fs.mountpoints, iter) {
        mount_t *mount = container_of(iter, mount_t, mountpoints);

        if (mount->fs == fs) {
            kdebug("mount point found! %s", mount->mountpoint->d_name);

            list_remove(&mount->mountpoints);
            kfree(mount->mountpoint);
            kfree(mount->fs);
            kfree(mount);

            return 0;
        }
    }

    return -ENOENT;
}

void vfs_list_mountpoints(void)
{
    list_head_t *iter;
    FOREACH(&root_fs.mountpoints, iter) {
        mount_t *tmp = container_of(iter, mount_t, mountpoints);
        kprint("'%s' | ", tmp->mountpoint->d_name);
    }
    kprint("\n");
}

dentry_t *vfs_lookup(const char *path)
{
    mount_t *mnt    = NULL;
    dentry_t *dntr  = NULL,
             *tmp   = NULL;
    char *mnt_point = vfs_get_mountpoint(path);

    kdebug("potential mount point is %s", mnt_point);

    list_head_t *iter;
    FOREACH(&root_fs.mountpoints, iter) {
        mnt = container_of(iter, mount_t, mountpoints);

        if (strncmp(mnt->mountpoint->d_name, mnt_point, VFS_NAME_MAX_LEN) == 0)
            break;

        mnt = NULL;
    }

    if (mnt == NULL) {
        mnt = &root_fs;
        mnt_point[0] = '/';
        mnt_point[1] = '\0';
    }

    if (mnt->fs == NULL) {
        kdebug("mountpoint '%s' doesn't have file system!", mnt_point);
        kfree(mnt_point);
        return NULL;
    }

    char *to_free, *r_path, *tok;
    inode_t *ret, *ino;

    to_free = r_path = strdup(path + strlen(mnt_point) + 1);
    ino  = mnt->mountpoint->d_inode;
    dntr = mnt->mountpoint;

    while ((tok = strsep(&r_path, "/")) != NULL) {
        /* dentry not found from the cache, use the actual file system to find it */
        if ((tmp = dentry_lookup(tok)) == NULL) {
            if ((dntr = tmp = ino->i_ops->lookup_dentry(mnt->fs, dntr, tok)) == NULL)
                break;
        }
        dntr = tmp;
    }
    kfree(to_free);

    if (!dntr)
        errno = ENOENT;
    return dntr;
}

file_t *vfs_open_file(dentry_t *dntr)
{
    if (!dntr) {
        errno = EINVAL;
        return NULL;
    }

    if (!dntr->d_inode->f_ops->open) {
        errno = ENOSYS;
        return NULL;
    }

    return dntr->d_inode->f_ops->open(dntr, VFS_READ);
}

void vfs_close_file(file_t *file)
{
    if (!file) {
        errno = EINVAL;
        return;
    }

    if (!file->f_ops->close) {
        errno = ENOSYS;
        return;
    }

    file->f_ops->close(file);
}

int vfs_seek(file_t *file, off_t off)
{
    if (!file) {
        errno = EINVAL;
        return -1;
    }

    if (!file->f_ops->seek) {
        errno = ENOSYS;
        return -1;
    }

    return file->f_ops->seek(file, off);
}

ssize_t vfs_read(file_t *file, off_t offset, size_t size, void *buffer)
{
    if (!file || !buffer)
        return -EINVAL;

    if (!file->f_ops->read)
        return -ENOSYS;

    return file->f_ops->read(file, offset, size, buffer);
}

ssize_t vfs_write(file_t *file, off_t offset, size_t size, void *buffer)
{
    if (!file || !buffer)
        return -EINVAL;

    if (!file->f_ops->write)
        return -ENOSYS;

    return file->f_ops->write(file, offset, size, buffer);
}
