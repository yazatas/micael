#include <lib/bitmap.h>
#include <kernel/kprint.h>
#include <mm/kheap.h>

#define BM_GET_MULTIPLE_OF_32(n) (n % 32) ? ((n / 32) + 1) : (n / 32)

bitmap_t *bm_alloc_bitmap(size_t nmemb)
{
    bitmap_t *bm;
    size_t num_bits;

    if ((bm = kmalloc(sizeof(bitmap_t))) == NULL) {
        return NULL;
    }

    if (nmemb == 0) {
        bm->bits = NULL;
        bm->len  = 0;
        return bm;
    }

    num_bits = BM_GET_MULTIPLE_OF_32(nmemb); /* round it up to nearest multiple of 32 */
    bm->len = num_bits * 32;

    if ((bm->bits = kcalloc(num_bits, sizeof(uint32_t))) == NULL) {
        return NULL;
    }

    return bm;
}

void bm_dealloc_bitmap(bitmap_t *bm)
{
    kfree(bm->bits);
    kfree(bm);
}

/* set nth bit */
int bm_set_bit(bitmap_t *bm, uint32_t n)
{
	if (n / 32 > bm->len) {
		return BM_RANGE_ERROR;
	}

	bm->bits[n / 32] |= 1 << (n % 32);
	return 0;
}

int bm_set_range(bitmap_t *bm, uint32_t n, uint32_t k)
{
    if (n >= bm->len || k >= bm->len) {
        return BM_RANGE_ERROR;
    }

    while (n <= k) {
        bm->bits[n / 32] |= 1 << (n % 32);
        n++;
    }

    return 0;
}

/* unset nth bit */
int bm_unset_bit(bitmap_t *bm, uint32_t n)
{
    if (n >= bm->len) {
        return BM_RANGE_ERROR;
    }

    bm->bits[n / 32] &= ~(1 << (n % 32));
    return 0;
}

int bm_unset_range(bitmap_t *bm, uint32_t n, uint32_t k)
{
    if (n >= bm->len || k >= bm->len) {
        return BM_RANGE_ERROR;
    }

    while (n <= k) {
        bm->bits[n / 32] &= ~(1 << (n % 32));
        n++;
    }

    return 0;
}

/* return BM_RANGE_ERROR on error and 0/1 on success */
int bm_test_bit(bitmap_t *bm, uint32_t n)
{
    if (n >= bm->len) {
        return BM_RANGE_ERROR;
    }

    return (bm->bits[n / 32] & (1 << (n % 32))) >> (n % 32);
}

static int bm_find_first(bitmap_t *bm, uint32_t n, uint32_t k, uint8_t bit_status)
{
    if (n >= bm->len || k >= bm->len) {
        return BM_RANGE_ERROR;
    }

    while (n <= k) {
        if (((bm->bits[n / 32] & (1 << (n % 32))) >> (n % 32)) == bit_status)
            return n;
        n++;
    }

    return BM_NOT_FOUND_ERROR;
}

int bm_find_first_unset(bitmap_t *bm, uint32_t n, uint32_t k)
{
    return bm_find_first(bm, n, k, 0);
}

int bm_find_first_set(bitmap_t *bm, uint32_t n, uint32_t k)
{
    return bm_find_first(bm, n, k, 1);
}

static int bm_find_first_range(bitmap_t *bm, uint32_t n, uint32_t k, size_t len, uint8_t bit_status)
{
    if (n >= bm->len || k >= bm->len) {
        return BM_RANGE_ERROR;
    }

    int start = BM_NOT_FOUND_ERROR;
    size_t cur_len = 0;

    while (n <= k) {
        if (((bm->bits[n / 32] & (1 << (n % 32))) >> (n % 32)) == bit_status) {

            if (start == BM_NOT_FOUND_ERROR)
                start = n;

            if (++cur_len == len)
                return start;

        } else {
            start   = BM_NOT_FOUND_ERROR;
            cur_len = 0;
        }
        n++;
    }

    return BM_NOT_FOUND_ERROR;
}

int bm_find_first_set_range(bitmap_t *bm, uint32_t n, uint32_t k, size_t len)
{
    return bm_find_first_range(bm, n, k, len, 1);
}

int bm_find_first_unset_range(bitmap_t *bm, uint32_t n, uint32_t k, size_t len)
{
    return bm_find_first_range(bm, n, k, len, 0);
}
