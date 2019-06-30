#ifndef __MMU_TYPES_H__
#define __MMU_TYPES_H__

#include <sys/types.h>
#include <lib/list.h>

#define PAGE_SIZE       4096
#define INVALID_ADDRESS ULONG_MAX
#define BUDDY_MAX_ORDER 16
#define PAGE_SHIFT      12

#define MM_SET_FLAG(value, flag)   (value |= flag)
#define MM_UNSET_FLAG(value, flag) (value &= ~flag)
#define MM_TEST_FLAG(value, flag)  (value & flag)

enum MM_PAGE_FLAGS {
    MM_PRESENT    = 1,
    MM_READWRITE  = 1 << 1,
    MM_READONLY   = 0 << 1,
    MM_USER       = 1 << 2,
    MM_WR_THROUGH = 1 << 3,
    MM_D_CACHE    = 1 << 4,
    MM_ACCESSED   = 1 << 5,
    MM_SIZE_4MB   = 1 << 6,
#ifdef __x86_64__
    MM_2MB        = 1 << 7, // TODO
#endif
    MM_COW        = 1 << 9,
};

typedef enum MM_ALLOC_FLAGS {
    MM_NO_FLAG = 0,
} mm_flags_t;

enum MM_ZONES {
    MM_ZONE_DMA,     /* 0MB  - 16MB */
    MM_ZONE_NORMAL,  /* 16MB - 2GB */
    MM_ZONE_HIGH,    /* 2GB  -  */
};

enum MM_ZONE_RANGES {
    MM_ZONE_DMA_START    = 0x0000000000000000,
    MM_ZONE_DMA_END      = 0x0000000001000000,
    MM_ZONE_NORMAL_START = 0x0000000001000000,
    MM_ZONE_NORMAL_END   = 0xffffffffffffffff,
    MM_ZONE_HIGH_START   = 0xffffffffffffffff,
    MM_ZONE_HIGH_END     = 0xffffffffffffffff,
};

enum MM_PAGE_TYPES {
    MM_PT_INVALID = 0 << 0,
    MM_PT_FREE    = 1 << 0,
    MM_PT_IN_USE  = 1 << 1,
};

typedef struct page {
    list_head_t list;
    uint8_t type:2;   /* type of memory (see MM_PAGE_TYPES) */
    uint8_t order:5;  /* order of block (0 - BUDDY_MAX_ORDER - 1) */
    uint8_t first:1;  /* is this the first block of a range? */
} page_t;

#endif /* __MMU_TYPES_H__ */
