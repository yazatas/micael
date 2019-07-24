#include <drivers/ioapic.h>
#include <drivers/lapic.h>
#include <kernel/acpi.h>
#include <kernel/compiler.h>
#include <kernel/io.h>
#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <mm/mmu.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>

enum FIXED_ACPI_FLAGS {
    /* WBINVD is correctly supported.
     * Signifies that the WBINVD instruction correctly flushes the processor caches,
     * maintains memory coherency, and upon completion of the instruction, all caches for
     * the current processor contain no cached data other than what the OS references and
     * allows to be cached. If this flag is not set, the ACPI OS is responsible for disabling
     * all ACPI features that need this function. */
    FA_WBINDV       = 1 << 0,

    /* If set, indicates that the hardware flushes all caches on the WBINVD instruction
     * and maintains memory coherency, but does not guarantee the caches are invalidated.
     * This provides the complete semantics of the WBINVD instruction, and provides enough
     * to support the system sleeping states. Note that on Intel Pentium Pro Processor machines,
     * the WBINVD instruction must flush and invalidate the caches.
     * If neither of the WBINVD flags are set, the system will require FLUSH_SIZE and FLUSH_STRIDE
     * to support sleeping states. If the FLUSH parameters are also not supported,
     * the machine cannot support sleeping states S1, S2, or S3. */
    FA_WBINDV_FLUSH = 1 << 1,

    /* A one indicates that the C1 power state is supported on all processors.
     * A system can support more Cx states, but is required to at least support the C1 power state. */
    FA_PROC_C1      = 1 << 2,

    /* A zero indicates that the C2 power state is configured to only work on a UP system.
     * A one indicates that the C2 power state is configured to work on a UP or MP system */
    FA_P_LVL2_UP    = 1 << 3,

    /* A zero indicates the power button is handled as a fixed feature programming model;
     * a one indicates the power button is handled as a control method device. If the system
     * does not have a power button, this value would be “1” and no sleep button device would be present */
    FA_PWR_BUTTON   = 1 << 4,

    /* A zero indicates the sleep button is handled as a fixed feature programming model;
     * a one indicates the power button is handled as a control method device. If the system
     * does not have a sleep button, this value would be “1” and no sleep button device would be present.  */
    FA_SLP_BUTTON   = 1 << 5,

    /* A zero indicates the RTC wake-up status is supported in fixed register space;
     * a one indicates the RTC wake-up status is not supported in fixed register space */
    FA_FIX_RTC      = 1 << 6,

    /* Indicates whether the RTC alarm function can wake the system from the S4 state.
     * The RTC must be able to wake the system from an S1, S2, or S3 sleep state.
     * The RTC alarm can optionally support waking the system from the S4 state, as indicated by this value. */
    FA_RTC_S4       = 1 << 7,

    /* A zero indicates TMR_VAL is implemented as a 24-bit value.
     * A one indicates TMR_VAL is implemented as a 32-bit value.
     * The TMR_STS bit is set when the most significant bit of the TMR_VAL toggles */
    FA_TMR_VAL_EXT  = 1 << 8,

    /* A zero indicates that the system cannot support docking.
     * A one indicates that the system can support docking.
     * Note that this flag does not indicate whether or not a docking station is currently present;
     * it only indicates that the system is capable of docking */
    FA_DCK_CAP      = 1 << 9,
};

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

/* Save the Fixed ACPI Descriptor Table to static global variable
 * because multiple functions of ACPI will need to use this data */
static struct fixed_acpi_desc_table {
    struct common_header hdr;

    /* Physical memory address (0 - 4GB) of the
     * Firmware ACPI Control Structure, where the OS
     * and Firwmare exchange control information */
    uint32_t fw_ctrl;

    /* Physical memory addrss (0 - 4GB) of the
     * Differentianted System Descriptor Table */
    uint32_t dsdt;

    /* The interrupt mode of the ACPI description. The SCI vector and Plug
     * and Play interrupt information assume some interrupt controller implementation
     * model for which the OS must also provide support. This value represents the interrupt
     * model being assumed in the ACPI description of the OS. This value therefore represents
     * the interrupt model. This value is not allowed to change for a given machine, even across reboots.
     *
     * 0: Dual PIC
     * 1: Multiple APIC */
    uint8_t int_model;

    uint8_t reserved1;

    /* Interrupt pin the SCI interrupt is wired to in in 8256 mode.
     * The OS is required to treat the ACPI SCI interrupt as sharable, level, active low interrupt  */
    uint16_t sci_int;

    /* System port address of the SMI Command Port. During ACPI OS initialization, the OS can
     * determine that the ACPI hardware registers are owned by SMI (by way of the SCI_EN bit), in
     * which case the ACPI OS issues the SMI_DISABLE command to the SMI_CMD
     * port. The SCI_EN bit effectively tracks the ownership of the ACPI hardware registers. The
     * OS issues commands to the SMI_CMD port synchronously from the boot processor. */
    uint32_t smi_cmd;

    /* The value to write to SMI_CMD to disable SMI ownership of the ACPI hardware registers. The
     * last action SMI does to relinquish ownership is to set the SCI_EN bit. The OS initialization process
     * will synchronously wait for the ownership transfer to complete, so the ACPI system releases
     * SMI ownership as timely as possible. */
    uint8_t acpi_enable;

    /* The value to write to SMI_CMD to re-enable SMI ownership of the ACPI hardware registers.
     * This can only be done when ownership was originally acquired from SMI by the OS using
     * ACPI_ENABLE. An OS can hand ownership back to SMI by relinquishing use to the ACPI
     * hardware registers, masking off all SCI interrupts, clearing the SCI_EN bit and then writing
     * ACPI_DISABLE to the SMI_CMD port from the boot processor. */
    uint8_t acpi_disable;

    /* The value to write to SMI_CMD to enter the S4BIOS state. The S4BIOS state provides an
     * alternate way to enter the S4 state where the firmware saves and restores the memory context.
     * A value of zero in S4BIOS_F indicates S4BIOS_REQ is not supported. */
    uint8_t s4bios_req;

    uint8_t reserved2;

    /* System port addresses of the Power Management Event and Control Register Blocks
     * Some of these may be 0 which means that the block/event is not supported */
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    uint32_t pm2_cnt_blk;
    uint32_t pm_tmr_blk; /* timer */

    /* Generic Purpose Event Register Blocks 0 and 1 */
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;

    /* Number of bytes in port address space decoded by various register blocks */
    uint8_t pm1_evt_len;
    uint8_t pm1_cnt_len;
    uint8_t pm2_cnt_len;
    uint8_t pm_tm_len;
    uint8_t gpe0_blk_len;
    uint8_t gpe1_blk_len;

    /* Offset within the ACPI general-purpose event model where GPE1 based events start. */
    uint8_t gpe1_base;

    uint8_t reserved3;

    /* The worst-case hardware latency, in microseconds, to enter and exit a C2 state.
     * A value > 100 indicates the system does not support a C2 state. */
    uint16_t p_lvl2_lat;

    /* The worst-case hardware latency, in microseconds, to enter and exit a C3 state.
     * A value > 1000 indicates the system does not support a C3 state.  */
    uint16_t p_lvl3_lat;

    /* If WBINVD=0, the value of this field is the number of flush strides that need to be read
     * (using cacheable addresses) to completely flush dirty lines from any processor’s memory caches.
     * Note that the value in FLUSH_STRIDE is typically the smallest cache line width on any of
     * the processor’s caches (for more information, see the FLUSH_STRIDE field definition). If the
     * system does not support a method for flushing the processor’s caches, then FLUSH_SIZE and
     * WBINVD are set to zero. Note that this method of flushing the processor caches has limitations,
     * and WBINVD=1 is the preferred way to flush the processors caches. In particular, it is known that
     * at least Intel Pentium Pro Processor, MP C3 support, 3rd level victim caches require
     * WBINVD=1 support. This value is typically at least 2 times the cache size. The maximum
     * allowed value for FLUSH_SIZE multiplied by FLUSH_STRIDE is 2 MB for a typical maximum
     * supported cache size of 1 MB. Larger cache sizes are supported using WBINVD=1.
     *
     * This value is ignored if WBINVD=1 */
    uint16_t flush_size;

    /* If WBINVD=0, the value of this field is the cache line width, in bytes, of the processor’s memory
     * caches. This value is typically the smallest cache line width on any of the processor’s caches. For
     * more information, see the description of the FLUSH_SIZE field.
     *
     * This value is ignored if WBINVD=1 */
    uint16_t flush_stride;

    /* The zero-based index of where the processor’s
     * duty cycle setting is within the processor’s P_CNT register.  */
    uint8_t duty_offset;

    /* The bit width of the processor’s duty cycle setting value in the P_CNT register.
     * Each processor’s duty cycle setting allows the software to select a nominal processor
     * frequency below its absolute frequency as defined by: THTL_EN = 1 BF * DC / (2DUTY_WIDTH) where:
     *  - BF = Base frequency
     *  - DC = Duty cycle setting
     *
     * When THTL_EN is 0, the processor runs at its absolute BF.
     * A DUTY_WIDTH value of 0 indicates that processor duty cycle is not supported and the processor
     * continuously runs at its base frequency. */
    uint8_t duty_width;

    /* The RTC CMOS RAM index to the day-of-month alarm value.
     * If this field contains a zero, then the RTC day of the month alarm feature is not supported.
     * If this field has a non-zero value, then this field contains an index into RTC RAM space that
     * the OS can use to program the day of the month alarm.
     *
     * See section 4.7.2.4 for a description of how the hardware works.  */
    uint8_t day_alarm;

    /* The RTC CMOS RAM index to the month of year alarm value.
     * If this field contains a zero, then the RTC month of the year alarm feature is not supported.
     * If this field has a non-zero value, then this field contains an index into RTC RAM space that
     * the OS can use to program the month of the year alarm. If this feature is supported,
     * then the DAY_ALRM feature must be supported also.  */
    uint8_t mon_alarm;

    /* The RTC CMOS RAM index to the century of data value (hundred and thousand year decimals).
     * If this field contains a zero, then the RTC centenary feature is not supported.
     * If this field has a non-zero value, then this field contains an index into RTC RAM space that
     * the OS can use to program the centenary field. */
    uint8_t century;

    uint8_t reserved4;

    /* Fixed feature flags. See FIXED_ACPI_FLAGS for a description of this field */
    uint32_t flags;

} *fadt = NULL;

static struct multiple_apic {
    struct common_header hdr;
    uint32_t local_apic_addr;
    uint32_t flags;
    uint32_t apics[0];
} *mapic = NULL;

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

int acpi_initialize(void)
{
    unsigned long *rspd_start = NULL;

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

#if 0
    kdebug("RSDT signature:    %s", table->hdr.signature);
    kdebug("RSDT length:       %u", table->hdr.length);
    kdebug("RSDT # of entries: %u", num_entries);
#endif

    for (uint32_t i = 0; i < num_entries; ++i) {
        struct desc_header *desc_hdr =
            (struct desc_header *)(unsigned long)table->entry_address[i];

        if (kstrncmp((char *)desc_hdr->hdr.signature, "FACP", 3) == 0)
            fadt = mmu_p_to_v((unsigned long)desc_hdr);

        if (kstrncmp((char *)desc_hdr->hdr.signature, "APIC", 3) == 0)
            mapic = mmu_p_to_v((unsigned long)desc_hdr);
    }

    if (fadt == NULL) {
        kdebug("Fixed ACPI entry not found!");
        return -ENXIO;
    }

    /* ACPI has already been enabled */
    if ((fadt->pm1a_cnt_blk & 0x1) == 1 &&
        fadt->acpi_enable          == 0 &&
        fadt->acpi_disable         == 0 &&
        fadt->smi_cmd              == 0)
    {
        return 0;
    }

    /* Write "ACPI Enable" byte to SMI and wait for the hardware to change mode */
    outb(fadt->smi_cmd, fadt->acpi_enable);

    while ((inw(fadt->pm1a_cnt_blk) & 1) == 0);

    return 0;
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

int acpi_parse_madt(void)
{
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

    return 0;
}
