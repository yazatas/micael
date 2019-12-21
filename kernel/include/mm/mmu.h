#ifndef __MMU_H__
#define __MMU_H__

#include <mm/types.h>
#include <sys/types.h>

typedef struct task task_t;

/* Initialize the Memory Management Unit:
 *  - native MMU
 *  - boot memory allocator
 *  - preinitialize heap and slab for booting
 *  - initialize page frame allocator (PFA)
 *  - reinitialize heap and slab using PFA
 *  
 * Return 0 on success 
 *
 * This function cannot fail in the traditional sense. 
 * If an error occurs in one the initialization function,
 * that will immediately result in a kernel panic because
 * they all are absolutely essential and one cannot work without the other*/
int mmu_init(void *arg);

/* TODO:  */
int mmu_map_page(unsigned long paddr, unsigned long vaddr, int flags);

/* TODO:  */
int mmu_unmap_page(unsigned long vaddr);

/* TODO:  */
int mmu_map_range(unsigned long pstart, unsigned long vstart, size_t n, int flags);

/* TODO:  */
int mmu_unmap_range(unsigned long start, size_t n);

/* TODO:  */
unsigned long mmu_v_to_p(void *vaddr);

/* TODO:  */
void *mmu_p_to_v(unsigned long paddr);

/* TODO:  */
void *mmu_build_dir(void);

/* TODO:  */
void mmu_destroy_dir(void *dir);

/* TODO: should this take something as parameter */
void *mmu_duplicate_dir(void);

void mmu_switch_ctx(task_t *task);

void mmu_walk_addr(void *addr);

#endif /* __MMU_H__ */
