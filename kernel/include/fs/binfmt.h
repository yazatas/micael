#ifndef __BIMFMT_H__
#define __BIMFMT_H__

#include <fs/fs.h>
#include <lib/list.h>

void binfmt_init(void);
void binfmt_add_loader(bool (*load_bin)(file_t *, int, char **));
void binfmt_load(file_t *file, int argc, char **argv);

/* defined in kernel/fs/binfmt_elf.c */
extern bool binfmt_elf_loader(file_t *file, int argc, char **argv);

#endif /* __BIMFMT_H__ */
