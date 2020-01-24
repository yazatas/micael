#include <drivers/ioapic.h>
#include <kernel/compiler.h>
#include <kernel/kprint.h>
#include <kernel/mp.h>
#include <kernel/util.h>
#include <sys/types.h>
#include <errno.h>

enum ENTRIES {
    MP_CPU         = 0,
    MP_BUS         = 1,
    MP_IOAPIC      = 2,
    MP_IOINT_ASSGN = 3,
    MP_LAPIC_ASSGN = 4
};

enum ENTRY_SIZES {
    MP_ES_CPU   = 20,
    MP_ES_IOINT = 8,
    MP_ES_OTHER = 8,
};

struct mp_fp {
    uint8_t signature[4];
    uint32_t mpconf_ptr;
    uint8_t length;
    uint8_t version;
    uint8_t checksum;
    uint8_t mp_feat1;
    uint8_t mp_feat2;
    uint8_t mp_feat3_5[3]; /* reserved for future(??) use */
} __packed;

struct mp_conf {
    uint8_t signature[4];
    uint16_t base_length;
    uint8_t revision;
    uint8_t checksum;
    uint64_t oem_id;
    uint8_t product_id[12];

    uint32_t oem_ptr;
    uint16_t oem_size;

    uint16_t entry_cnt;
    uint32_t lapic_addr;

    uint16_t ext_tbl_len;
    uint8_t  ext_tbl_checksum;
    uint8_t  unused;
} __packed;

struct mp_ioint {
    uint8_t type;
    uint16_t flags;
    uint8_t bus_id;
    uint8_t bus_irq;
    uint8_t ioapic_id;
    uint8_t ioapic_irq;
} __packed;

static unsigned long *__find_mp_start(unsigned long start, unsigned long end)
{
    const char *needle = "_MP_";
    const size_t nlen  = kstrlen(needle);

    for (size_t i = start; i < end; i += nlen) {
        uint8_t *ptr = (uint8_t *)i;

        for (size_t k = 0; k < nlen; ++k) {
            if (ptr[k] != needle[k])
                goto end;
        }

        return (unsigned long *)i;
end:;
    }

    return NULL;
}

int mp_init(void)
{
    unsigned long *mp_start = NULL;

    if ((mp_start = __find_mp_start(0x00080000, 0x0009ffff)) == NULL) {
        kdebug("MP start not found from Extended BIOS Data Area!");

        if ((mp_start = __find_mp_start(0x000f0000, 0x000fffff)) == NULL) {
            kdebug("MP start not upper memory (0x%x - 0x%x", 0x000f0000, 0x000fffff);
            return -ENXIO;
        }
    }

    struct mp_fp *mp_fp  = (struct mp_fp *)mp_start;
    struct mp_conf *conf = (struct mp_conf *)((unsigned long)mp_fp->mpconf_ptr);
    uint8_t *hdr         = (uint8_t *)conf + sizeof(struct mp_conf);

    if (kmemcmp((char *)conf->signature, "PCMP", 4) != 0)
        return -EINVAL;

    /* Because we've used ACPI to initialize all LAPIC/IOAPIC stuff
     * we can skip it here and just parse the I/O interrupt assignments
     * that are needed for PCI support */
    for (size_t i = 0; i < conf->entry_cnt; ++i) {
        switch (*hdr) {
            case MP_CPU:
                hdr += MP_ES_CPU;
                break;

            case MP_BUS:
            case MP_IOAPIC:
            case MP_LAPIC_ASSGN:
                hdr += MP_ES_OTHER;
                break;

            case MP_IOINT_ASSGN:
            {
                struct mp_ioint *ioint = (struct mp_ioint *)hdr;

                ioapic_assign_ioint(
                    ioint->bus_id, ioint->bus_irq,
                    ioint->ioapic_id, ioint->ioapic_irq
                );
                hdr += MP_ES_IOINT;
            }
            break;

            default:
                return -EINVAL;
        }
    }

    return 0;
}
