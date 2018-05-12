#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <kernel/common.h>

typedef struct bitmap {
	size_t len;
	uint32_t *bits;
} bitmap_t;


// TODO create (de)allocation api for bitmap

int bm_set_bit(bitmap_t *bm, uint32_t n);
int bm_unset_bit(bitmap_t *bm, uint32_t n);

/* set values from n to k to 1/0 */
int bm_set_range(bitmap_t *bm, uint32_t n, uint32_t k);
int bm_unset_range(bitmap_t *bm, uint32_t n, uint32_t k);

int bm_test_bit(bitmap_t *bm, uint32_t n);

#endif /* end of include guard __BITMAP_H__ */
