#include <kernel/io.h>
#include <drivers/ioapic.h>
#include <kernel/common.h>
#include <kernel/isr.h>
#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <kernel/pic.h>
#include <mm/mmu.h>

#define MAX_IOAPIC         20

#define IOAPIC_IOREGSEL   0x00
#define IOAPIC_IOWIN      0x10
#define IOAPIC_IOAPICID   0x00
#define IOAPIC_IOAPICVER  0x01
#define IOAPIC_IOREDTBL   0x10

#define IOAPIC_REG_ID     0x00  /* id */
#define IOAPIC_REG_VER    0x01  /* version */
#define IOAPIC_REG_TABLE  0x10  /* redirection table */

#define IOAPIC_INT_DISABLED   0x00010000
#define IOAPIC_INT_LEVEL      0x00008000
#define IOAPIC_INT_ACTIVELOW  0x00002000
#define IOAPIC_INT_LOGICAL    0x00000800

static struct {
    int num_apics;

    struct {
        uint8_t  id;
        uint8_t *base;
        uint8_t *mapped;
        uint32_t intr_base;
    } apics[MAX_IOAPIC];

} io_apic;

/* I/O APIC is programmed by first writing the wanted register to base address 
 * and then reading/writing from/to the IOWIN data register */
static uint32_t __read_reg(uint8_t *base, uint32_t reg)
{
    write_32(base + IOAPIC_IOREGSEL, reg);
    return read_32(base + IOAPIC_IOWIN);
}

static void __write_reg(uint8_t *base, uint32_t reg, uint32_t value)
{
    write_32(base + IOAPIC_IOREGSEL, reg);
    write_32(base + IOAPIC_IOWIN,    value);
}

void ioapic_initialize_all(void)
{
    kassert(io_apic.num_apics > 0);

    /* disable 8259 PIC by masking all interrupts */
    outb(PIC_MASTER_DATA_PORT, 0xff);
    outb(PIC_SLAVE_DATA_PORT,  0xff);

    /* Initialize all I/O APICs found on the bus and initially disable all IRQs */
    for (int i = 0; i < io_apic.num_apics; ++i) {
        uint8_t *ioapic_v = (uint8_t *)io_apic.apics[i].base;
        mmu_map_page((unsigned long)ioapic_v, (unsigned long)ioapic_v, MM_PRESENT | MM_READWRITE);

        uint32_t id  = __read_reg(ioapic_v, IOAPIC_REG_ID);
        uint32_t ver = __read_reg(ioapic_v, IOAPIC_REG_VER);

        /* Disabling the interrupt happens in two writes:
         *  - first write  (offset 0) disables the irq line itself
         *  - second write (offset 1) sets the destination cpu of disabled irq to 0 */
        for (size_t intr = 0; intr < ((ver >> 16) & 0xff); ++intr) {
            unsigned offset = IOAPIC_REG_TABLE + 2 * intr + io_apic.apics[i].intr_base;

            __write_reg(ioapic_v, offset + 0, IOAPIC_INT_DISABLED | (VECNUM_IRQ_START + intr));
            __write_reg(ioapic_v, offset + 1, 0);
        }
    }
}

void ioapic_register_dev(uint8_t ioapic_id, uint32_t ioapic_addr, uint32_t intr_base)
{
    kassert(ioapic_id < MAX_IOAPIC);

    io_apic.apics[ioapic_id].id        = ioapic_id;
    io_apic.apics[ioapic_id].base      = (uint8_t *)(unsigned long)ioapic_addr;
    io_apic.apics[ioapic_id].intr_base = intr_base;

    io_apic.num_apics++;
}

void ioapic_enable_irq(unsigned cpu, unsigned irq)
{
    unsigned offset   = IOAPIC_REG_TABLE + 2 * (irq - VECNUM_IRQ_START);
    uint8_t *ioapic_v = (uint8_t *)io_apic.apics[cpu].base;
    mmu_map_page((unsigned long)ioapic_v, (unsigned long)ioapic_v, MM_PRESENT | MM_READWRITE);

    __write_reg(ioapic_v, offset + 0, irq);
    __write_reg(ioapic_v, offset + 1, cpu << 24);
}

void ioapic_assign_ioint(uint8_t bus_id, uint8_t bus_irq, uint8_t ioapic_id, uint8_t ioapic_irq)
{
    kprint("ioapic - assign i/o interrupt for 0x%x:0x%x to 0x%x:0x%x\n",
            bus_id, bus_irq, ioapic_id, ioapic_irq);

    /* TODO:  */
}

unsigned long ioapic_get_base(void)
{
    kassert(io_apic.num_apics == 1);

    return (unsigned long)io_apic.apics[0].base;
}
