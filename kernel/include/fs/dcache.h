#ifndef __DCACHE_H__
#define __DCACHE_H__

#include <fs/fs.h>

int dcache_init(void);

dentry_t *dentry_alloc(const char *name);
void dentry_dealloc(dentry_t *dntr);

dentry_t *dentry_lookup(const char *dname);

#endif /* __DCACHE_H__ */
