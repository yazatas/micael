#ifndef __MP_H__
#define __MP_H__

/* Search for MP Floating Point and all MP-related structures
 * from memory, parse MP Configuration table entries
 * and set IRQ redirection entries correctly for I/O APICs
 *
 * Return 0 on success
 * Return -ENXIO if MP Floating Point Structure is not found
 * Return -EINVAL if one of checksums/signatures is invalid */
int mp_init(void);

#endif /* __MP_H__ */
