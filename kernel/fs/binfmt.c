#include <fs/binfmt.h>
#include <kernel/kpanic.h>
#include <mm/heap.h>

typedef struct {
    bool (*load_bin)(file_t *, int, char **);
    list_head_t list;
} binfmt_t;

static binfmt_t loaders;

void binfmt_init(void)
{
    loaders.load_bin = NULL;
    list_init_null(&loaders.list);
}

void binfmt_add_loader(bool (*load_bin)(file_t *, int, char **argv))
{
    binfmt_t *bfmt = kmalloc(sizeof(binfmt_t));
    bfmt->load_bin = load_bin;

    list_init_null(&bfmt->list);
    list_append(&loaders.list, &bfmt->list);
}

void binfmt_load(file_t *file, int argc, char **argv)
{
    list_head_t *iter = NULL;
    binfmt_t *loader  = NULL;

    FOREACH(&loaders.list, iter) {
        loader = container_of(iter, binfmt_t, list);

        if (loader->load_bin && loader->load_bin(file, argc, argv))
            kpanic("binfmt load_bin returned!");
    }

    kpanic("no suitable binfmt loader found!");
}
