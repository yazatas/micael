#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <kernel/common.h>

#define BM_BIT_SET             1
#define BM_BIT_NOT_SET         0
#define BM_OUT_OF_RANGE_ERROR -1
#define BM_NOT_FOUND_ERROR    -2

typedef struct bitmap {
    size_t len;
    uint32_t *bits;
} bitmap_t;


bitmap_t *bm_alloc_bitmap(size_t nmemb);
void      bm_dealloc_bitmap(bitmap_t *bm);

int bm_set_bit(bitmap_t *bm, uint32_t n);
int bm_unset_bit(bitmap_t *bm, uint32_t n);

/* set values from n to k to 1/0 */
int bm_set_range(bitmap_t *bm, uint32_t n, uint32_t k);
int bm_unset_range(bitmap_t *bm, uint32_t n, uint32_t k);

int bm_test_bit(bitmap_t *bm, uint32_t n);
int bm_find_first_unset(bitmap_t *bm, uint32_t n, uint32_t k);

#endif /* end of include guard __BITMAP_H__ */
