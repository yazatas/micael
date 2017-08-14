#ifndef __PFA_H__
#define __PFA_H__

#include <stdint.h>

#define PF_SIZE             0x1000
#define KALLOC_NO_MEM_ERROR 0xffffffff

typedef uint32_t pageframe_t;

void kfree_frame(pageframe_t frame);
pageframe_t kalloc_frame(void);

#endif /* end of include guard: __PFA_H__ */
