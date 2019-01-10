#include <fs/binfmt.h>
#include <kernel/kpanic.h>
#include <mm/heap.h>

typedef struct {
    binfmt_loader_t loader;
    list_head_t list;
} binfmt_t;

static binfmt_t loaders;

void binfmt_init(void)
{
    loaders.loader = NULL;
    list_init_null(&loaders.list);
}

void binfmt_add_loader(binfmt_loader_t loader)
{
    binfmt_t *bfmt = kmalloc(sizeof(binfmt_t));
    bfmt->loader = loader;

    list_init_null(&bfmt->list);
    list_append(&loaders.list, &bfmt->list);
}

bool binfmt_load(file_t *file, int argc, char **argv)
{
    list_head_t *iter = NULL;
    binfmt_t *loader  = NULL;

    FOREACH(&loaders.list, iter) {
        loader = container_of(iter, binfmt_t, list);

        if (loader->loader && loader->loader(file, argc, argv))
            kpanic("binfmt load_bin returned!");
    }

    return false;
}
