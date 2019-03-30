#ifndef __CHAR_DEV_H__
#define __CHAR_DEV_H__

#include <fs/fs.h>
#include <sys/types.h>

typedef struct cdev cdev_t;
file_ops_t *cdev_ops;

struct cdev {
    char *c_name;
    dev_t c_dev;
    file_ops_t *c_ops;

    list_head_t c_list; /* list of character devices */
};

/* initialize cache and bitmap objects for character drivers 
 * return 0 on success and -1 on error */
int cdev_init(void);

cdev_t *cdev_alloc(char *name, file_ops_t *ops, int major);
void cdev_dealloc(cdev_t *dev);

#endif /* __CHAR_DEV_H__ */
