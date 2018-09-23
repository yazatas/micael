#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stdint.h>
#include <stddef.h>

typedef struct fs fs_t;

enum {
    BM_BIT_SET            =  1,
    BM_BIT_NOT_SET        =  0,
    BM_OUT_OF_RANGE_ERROR = -1,
    BM_NOT_FOUND_ERROR    = -2,
    BM_RANGE_ERROR        = -3,
} BM_STATUS;

typedef struct bitmap {
    size_t len; /* TODO: this should be capacity not len */
    uint32_t *bits;
} bitmap_t;

#define TO_BM_LEN(n)    ((n) / 32)
#define BM_GET_SIZE(bm) (sizeof(*bm) + sizeof(uint32_t) * (bm->len / 32))

bitmap_t *bm_alloc_bitmap(size_t nmemb);
void      bm_dealloc_bitmap(bitmap_t *bm);

int bm_set_bit(bitmap_t *bm, uint32_t n);
int bm_unset_bit(bitmap_t *bm, uint32_t n);

/* set values from n to k to 1/0 */
int bm_set_range(bitmap_t *bm, uint32_t n, uint32_t k);
int bm_unset_range(bitmap_t *bm, uint32_t n, uint32_t k);

int bm_test_bit(bitmap_t *bm, uint32_t n);
int bm_find_first_set(bitmap_t *bm, uint32_t n, uint32_t k);
int bm_find_first_unset(bitmap_t *bm, uint32_t n, uint32_t k);

/* return the start bit of set range
 * on success and an error code (< 0) on error */
int bm_find_first_set_range(bitmap_t *bm, uint32_t n, uint32_t k, size_t len);

/* return the start bit of unset set range
 * on success and an error code (< 0) on error */
int bm_find_first_unset_range(bitmap_t *bm, uint32_t n, uint32_t k, size_t len);

#endif /* end of include guard __BITMAP_H__ */
