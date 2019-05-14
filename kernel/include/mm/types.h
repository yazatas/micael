#ifndef __MMU_TYPES_H__
#define __MMU_TYPES_H__

#include <sys/types.h>

#define PAGE_SIZE 4096

typedef struct task task_t;

typedef uint32_t page_t;

/* typedef struct page { */
/*     unsigned long address; /1* physical address of the page *1/ */
/*     int flags;             /1* flags associated with the page *1/ */
/* } page_t; */

typedef struct pdir {

} pdir_t;

#ifdef __x86_64__
typedef struct pdpt {

} pdpt_t;

typedef struct pml4 {

} pml4_t;
#endif

typedef struct cr3 {
    unsigned long address; /* address of the directory (TODO: physical or virtual?) */
    task_t *task;          /* owner of the page directory */

#ifdef __x86_64__
    pml4_t *dir;
#else
    pdir_t *dir;
#endif
} cr3_t;

#endif /* __MMU_TYPES_H__ */
