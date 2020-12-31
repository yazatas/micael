#include <kernel/acpi/acpica/acpi.h>
#include <drivers/ioapic.h>
#include <drivers/lapic.h>
#include <kernel/acpi/acpi.h>
#include <kernel/compiler.h>
#include <kernel/io.h>
#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <mm/mmu.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>

enum MULTIPLE_APIC_TYPES {
    MA_LOCAL_APIC        = 0,
    MA_IO_APIC           = 1,
    MA_INT_SRC_OVRRD     = 2,
    MA_NMI               = 3,
    MA_LOCAL_APIC_OVERRD = 4,
};

/* Root System Descriptor Pointer */
struct rspd_desc {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rstd_addr;
} __packed;

/* Root System Descriptor Pointer (for ACPI version >= 2.0) */
struct rspd_desc_2 {
    struct rspd_desc desc;

    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __packed;

/* Common header for RSDT, Descriptor Header
 * and Fixed ACPI Descriptor Table */
struct common_header {
    uint8_t signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t oem_table_id[8];
    uint32_t oem_revision;
    uint8_t creator_id;
    uint32_t creator_revision;
} __packed;

/* Root System Descriptor Table Descriptor */
struct rspd_table {
    struct common_header hdr;
    uint32_t entry_address[0];
};

struct desc_header {
    struct common_header hdr;
} __packed;

struct apic_common_header {
    uint8_t type;
    uint8_t length;
};

struct local_apic {
    struct apic_common_header hdr;
    uint8_t acpi_cpu_id;
    uint8_t apic_id;
    uint32_t flags;
} __packed;

struct io_apic {
    struct apic_common_header hdr;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_addr;
    uint32_t global_sys_intr_base;
} __packed;

struct int_src_override {
    struct apic_common_header hdr;
    uint8_t bus_src;
    uint8_t irq_src;
    uint32_t global_sys_intr;
    uint16_t flags;
} __packed;

/* non-maskable interrupts */
struct nmi {
    struct apic_common_header hdr;
    uint8_t acpi_cpu_id; /* 0xff means all cpus */
    uint16_t flags;
    uint8_t lint;        /* 0 or 1 */
} __packed;

struct local_apic_addr_override {
    struct apic_common_header hdr;
    uint16_t reserved;
    uint64_t local_apic_addr; /* 64-bit address */
} __packed;

static struct multiple_apic {
    struct common_header hdr;
    uint32_t local_apic_addr;
    uint32_t flags;
    uint32_t apics[0];
} *mapic = NULL;

static void *rspd_start = NULL;

static int __acpica_init(void)
{
    ACPI_STATUS status;

    if (ACPI_FAILURE(status = AcpiInitializeSubsystem())) {
        kdebug("Failed to initialize ACPI subsystem");
        return -ENOTSUP;
    }

    if (ACPI_FAILURE(status = AcpiInitializeTables(NULL, 0, false))) {
        kdebug("Failed to initialize ACPI tables");
        return -ENOTSUP;
    }

    if (ACPI_FAILURE(status = AcpiLoadTables())) {
        kdebug("Failed to load ACPI tables: %s", AcpiFormatException(status));
        return -ENOTSUP;
    }

    if (ACPI_FAILURE(status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION))) {
        kdebug("Failed to enable ACPI subsystem: %s", AcpiFormatException(status));
        return -ENOTSUP;
    }

    if (ACPI_FAILURE(status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION))) {
        kdebug("Failed to enable ACPI subsystem: %s", AcpiFormatException(status));
        return -ENOTSUP;
    }

    return 0;
}

/* return pointer to the start or RSD Descriptor if found, NULL otherwise */
static unsigned long *__find_rspd_start(unsigned long start, unsigned long end)
{
    const char *needle = "RSD PTR ";
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

static bool __validate_rspd_description(struct rspd_desc *desc)
{
    kassert(desc != NULL);

    size_t total_size = 0;
    uint8_t *ptr      = (uint8_t *)desc;

    for (size_t i = 0; i < sizeof(struct rspd_desc); ++i) {
        total_size += ptr[i];
    }

    if (desc->revision == 0) {
        kdebug("ACPI version 1.0 detected!");
    } else {
        /* TODO: calculate size of secondary descriptor and add its size to total_size */
        kdebug("ACPI version 2.0 or higher detected!");
    }

    return ((total_size & 0xff) == 0);
}

int acpi_init(void)
{
    if ((rspd_start = __find_rspd_start(0x00080000, 0x0009FFFF)) == NULL) {
        kdebug("RSDP start not found from Extended BIOS Data Area!");

        if ((rspd_start = __find_rspd_start(0x000E0000, 0x000FFFFF)) == NULL) {
            kdebug("RSDP start not Upper memory (0x%x - 0x%x", 0x000E0000, 0x000FFFFF);
            return -ENXIO;
        }
    }

    /* RSDP start has been found, now validate the structure and initialize ACPI */
    struct rspd_desc *desc = (struct rspd_desc *)rspd_start;

    if (!__validate_rspd_description(desc)) {
        kdebug("RSDP Descriptor from address 0x%x is not valid!", rspd_start);
        return -EINVAL;
    }

    struct rspd_table *table = (struct rspd_table *)(unsigned long)desc->rstd_addr;
    size_t num_entries       = (table->hdr.length - 36) / 4;

    for (uint32_t i = 0; i < num_entries; ++i) {
        struct desc_header *desc_hdr =
            (struct desc_header *)(unsigned long)table->entry_address[i];

        if (kstrncmp((char *)desc_hdr->hdr.signature, "APIC", 3) == 0)
            mapic = mmu_p_to_v((unsigned long)desc_hdr);
    }

    if (mapic == NULL) {
        kdebug("Multiple APIC not found!");
        return -EINVAL;
    }

    struct apic_common_header *hdr;
    uint8_t *ptr = (uint8_t *)mapic->apics;

    while (ptr < ((uint8_t *)mapic + mapic->hdr.length)) {
        hdr = (void *)ptr;

        switch (hdr->type) {
            case MA_LOCAL_APIC:
                lapic_register_dev(
                    ((struct local_apic *)ptr)->acpi_cpu_id,
                    ((struct local_apic *)ptr)->apic_id
                );
                break;

            case MA_IO_APIC:
                ioapic_register_dev(
                    ((struct io_apic *)ptr)->io_apic_id,
                    ((struct io_apic *)ptr)->io_apic_addr,
                    ((struct io_apic *)ptr)->global_sys_intr_base
                );
                break;

            case MA_INT_SRC_OVRRD:
                /* kdebug("interrupt source override"); */
                break;

            case MA_NMI:
                /* kdebug("non-maskable interrupt"); */
                break;

            case MA_LOCAL_APIC_OVERRD:
                /* kdebug("local apic address override"); */
                break;
        }

        ptr += hdr->length;
    }

    return __acpica_init();
}

unsigned long acpi_get_local_apic_addr(void)
{
    if (mapic == NULL) {
        kdebug("Multiple APIC Descriptor Table not found!");
        errno = ENXIO;
        return INVALID_ADDRESS;
    }

    return mapic->local_apic_addr;
}
