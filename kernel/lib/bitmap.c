#include <lib/bitmap.h>
#include <kernel/kprint.h>

/* set nth bit */
int bm_set_bit(bitmap_t *bm, uint32_t n)
{
	if (n / 32 > bm->len) {
		kdebug("bit %u is over range!", n);
		return -1;
	}

	bm->bits[n / 32] |= 1 << (n % 32);
	return 0;
}

int bm_set_range(bitmap_t *bm, uint32_t n, uint32_t k)
{
	if (n / 32 > bm->len || k / 32 > bm->len) {
		kdebug("argument is over range!");
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
	if (n / 32 > bm->len) {
		kdebug("bit %u is over range!", n);
		return -1;
	}

	bm->bits[n / 32] &= ~(1 << (n % 32));
	return 0;
}

int bm_unset_range(bitmap_t *bm, uint32_t n, uint32_t k)
{
	if (n / 32 > bm->len || k / 32 > bm->len) {
		kdebug("argument is over range!");
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
	if (n / 32 > bm->len) {
		kdebug("bit %u is over range!", n);
		return -1;
	}
	return (bm->bits[n / 32] & (1 << (n % 32)));
}
