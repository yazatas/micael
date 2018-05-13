#include <lib/bitmap.h>
#include <kernel/kprint.h>
#include <mm/kheap.h>

#define BM_GET_MULTIPLE_OF_32(n) (n % 32) ? ((n / 32) + 1) : (n / 32)

bitmap_t *bm_alloc_bitmap(size_t nmemb)
{
    bitmap_t *bm;
    size_t n;

    if ((bm = kmalloc(sizeof(bitmap_t))) == NULL)
        return NULL;

    n = BM_GET_MULTIPLE_OF_32(nmemb);
    bm->len = n * 32;

    if ((bm->bits = kcalloc(n, sizeof(uint32_t))) == NULL)
        return NULL;

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
    if (n >= bm->len) {
        kdebug("bit %u is over range!", n);
        return -1;
    }

    bm->bits[n / 32] |= 1 << (n % 32);
    return 0;
}

int bm_set_range(bitmap_t *bm, uint32_t n, uint32_t k)
{
    if (n >= bm->len || k >= bm->len) {
        kdebug("argument n(%u) or k(%u) is over range(%u)!", n, k, bm->len);
        return -1;
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
        kdebug("bit %u is over range!", n);
        return -1;
    }

    bm->bits[n / 32] &= ~(1 << (n % 32));
    return 0;
}

int bm_unset_range(bitmap_t *bm, uint32_t n, uint32_t k)
{
    if (n >= bm->len || k >= bm->len) {
        kdebug("argument n(%u) or k(%u) is over range(%u)!", n, k, bm->len);
        return -1;
    }

    while (n <= k) {
        bm->bits[n / 32] &= ~(1 << (n % 32));
        n++;
    }
    return 0;
}

/* return -1 on error and 0/1 on success */
int bm_test_bit(bitmap_t *bm, uint32_t n)
{
    if (n >= bm->len) {
        kdebug("bit %u is over range!", n);
        return -1;
    }
    return (bm->bits[n / 32] & (1 << (n % 32))) >> (n % 32);
}
