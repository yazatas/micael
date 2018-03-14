# Memory management in micael

## Kernel memory layout

Kernel is loaded to physical address 0x00100000 and virtual address 0xc0100000.

| start address | reserved for |
| --------------| :------------|
| 0xc0000000    | kernel code, data, bss |
| 0xd0000000    | kernel heap |
| 0xe0000000    | free page frames for miscellaneous use |

MMU defines a new data type: pageframe_t which holds a physical address of a 4K page.

# API 

## VMM

## Heap
`void *kmalloc(size_t size)`

Allocate a block of memory from the heap of size <size>. Return pointer to the memory.

NOTE! kmalloc never returns a NULL pointer. If kernel heap is out of memory, a kernel panic occurs.



`void *kcalloc(size_t nmemb, size_t size)`

Allocate a block of memory from the heap of size <size> * <nmemb>. Return pointer to the memory.



`void *krealloc(void *ptr, size_t size)`

Reallocate a block of memory. 



`void kfree(void *ptr)`

NOTE! ptr MUST point to the beginning of memory
