#include <drivers/lapic.h>
#include <drivers/pit.h>
#include <kernel/acpi.h>
#include <kernel/common.h>
#include <kernel/idt.h>
#include <kernel/io.h>
#include <kernel/isr.h>
#include <kernel/kassert.h>
#include <kernel/tick.h>
#include <mm/mmu.h>
#include <mm/types.h>
#include <errno.h>

/* Local APIC general defines */
#define IA32_APIC_BASE          0x0000001b  /* MSR index */
#define IA32_LAPIC_MSR_BASE     0xfffff000  /* Base addr mask */
#define IA32_LAPIC_MSR_ENABLE   0x00000800  /* Global enable */
#define IA32_LAPIC_MSR_BSP      0x00000100  /* BSP */

/* Local APIC register related defines */
#define LAPIC_REG_ID       0x0020  /* Local APIC id  */
#define LAPIC_REG_VER      0x0030  /* Local APIC version  */
#define LAPIC_REG_TPR      0x0080  /* task priority  */
#define LAPIC_REG_APR      0x0090  /* arbitration priority  */
#define LAPIC_REG_PPR      0x00a0  /* processor priority  */
#define LAPIC_REG_EOI      0x00b0  /* end of interrupt  */
#define LAPIC_REG_RRD      0x00c0  /* remote read  */
#define LAPIC_REG_LDR      0x00d0  /* logical destination  */
#define LAPIC_REG_DFR      0x00e0  /* destination format  */
#define LAPIC_REG_SVR      0x00f0  /* spurious interrupt  */
#define LAPIC_REG_ISR      0x0100  /* in-service */
#define LAPIC_REG_TMR      0x0180  /* trigger mode */
#define LAPIC_REG_IRR      0x0200  /* interrupt request */
#define LAPIC_REG_ESR      0x0280  /* error status  */
#define LAPIC_REG_CMCI     0x02f0  /* LVT (CMCI) */
#define LAPIC_REG_ICR_LO   0x0300  /* int command upper half */
#define LAPIC_REG_ICR_HI   0x0310  /* int command lower half */
#define LAPIC_REG_TIMER    0x0320  /* LVT (timer) */
#define LAPIC_REG_THERMAL  0x0330  /* LVT (thermal) */
#define LAPIC_REG_PMC      0x0340  /* LVT (performance counter) */
#define LAPIC_REG_LINT0    0x0350  /* LVT (LINT0) */
#define LAPIC_REG_LINT1    0x0360  /* LVT (LINT1) */
#define LAPIC_REG_ERROR    0x0370  /* LVT (error) */
#define LAPIC_REG_ICR      0x0380  /* timer Initial count  */
#define LAPIC_REG_CCR      0x0390  /* timer current count  */
#define LAPIC_REG_CFG      0x03e0  /* timer divide config  */

/* Local APIC vector related defines */

/* delivery mode */
#define LAPIC_VECTOR       0x000ff  /* vector number mask */
#define LAPIC_DM_FIXED     0x00000  /* delivery mode: fixed */
#define LAPIC_DM_LOWEST    0x00100  /* delivery mode: lowest */
#define LAPIC_DM_SMI       0x00200  /* delivery mode: SMI */
#define LAPIC_DM_NMI       0x00400  /* delivery mode: NMI */
#define LAPIC_DM_INIT      0x00500  /* delivery mode: INIT */
#define LAPIC_DM_STARTUP   0x00600  /* delivery mode: startup */
#define LAPIC_DM_EXT       0x00700  /* delivery mode: ExtINT */

/* delivery status */
#define LAPIC_DS_IDLE      0x00000  /* delivery status: idle */
#define LAPIC_DS_PEND      0x01000  /* delivery status: pend */

/* polarity etc. */
#define LAPIC_POL_HIGH      0x00000  /* polarity: High */
#define LAPIC_POL_LOW       0x02000  /* polarity: Low */
#define LAPIC_REMOTE        0x04000  /* remote IRR */
#define LAPIC_LVL_DEASSERT  0x00000  /* level: de-assert */
#define LAPIC_LVL_ASSERT    0x04000  /* level: assert */
#define LAPIC_TM_EDGE       0x00000  /* trigger mode: Edge */
#define LAPIC_TM_LEVEL      0x08000  /* trigger mode: Level */

/* interrupt disabled */
#define LAPIC_INT_DISABLED_MASK  0x10000     /* interrupt disabled */

/* Local APIC spurious-interrupt register bits */
#define LAPIC_SVR_ENABLE  0x00000100

/* Local APIC timer register */
#define LAPIC_TMR_ONESHOT      0x00000000  /* Timer mode: one-shot */
#define LAPIC_TMR_PERIODIC     0x00020000  /* Timer mode: periodic */
#define LAPIC_TMR_DEADLINE     0x00040000  /* Timer mode: tsc-deadline */

static struct {
    unsigned cpu_id;
    unsigned lapic_id;
} lapics[MAX_CPU];

static uint8_t *lapic_base     = NULL;
static unsigned cpu_count      = 0;    /* Number of CPUs */
static unsigned cpu_init_count = 0;    /* Number of initialized CPUs */

static void __configure_timer(void)
{
    uint16_t ticks;
    uint32_t end;

    write_32(lapic_base + LAPIC_REG_CFG, 0x0b);
    write_32(lapic_base + LAPIC_REG_ICR, 0xffffffff);

    /* Configure PIT */
    outb(PIT_CMD,    0xb6);
    outb(PIT_DATA_2, 0xff);
    outb(PIT_DATA_2, 0xff);
    outb(PIT_CHNL_2, inb(PIT_CHNL_2) | 0x1);

    do {
        outb(PIT_CMD, 0xe8);
    } while ((inb(PIT_DATA_2) & 0x80) == 0x80);

    /* sleep 50ms */
    do {
        outb(PIT_CMD, 0x80);
        ticks  =  inb(PIT_DATA_2);
        ticks |= (inb(PIT_DATA_2) << 8);
    } while (ticks > (2 * 65535 - 119318 + 10));

    end = read_32(lapic_base + LAPIC_REG_CCR);
    outb(PIT_CHNL_2, inb(PIT_CHNL_2) & ~0x01);

    write_32(lapic_base + LAPIC_REG_TIMER, LAPIC_TMR_PERIODIC | VECNUM_TIMER);
    write_32(lapic_base + LAPIC_REG_CFG, 0x0b);
    write_32(lapic_base + LAPIC_REG_ICR, ((0xffffffff - end) * 20) / 1000);

    tick_init((0xffffffff - end) * 20, 1000);
}

static void __svr_handler(isr_regs_t *cpu)
{
    (void)cpu;
    write_32(lapic_base + LAPIC_REG_EOI, 0);

    /* TODO: do something here? */
}

static void __tmr_handler(isr_regs_t *cpu)
{
    (void)cpu;

    write_32(lapic_base + LAPIC_REG_EOI, 0);
    tick_inc();
}

void lapic_initialize(void)
{
    unsigned long lapic_addr = acpi_get_local_apic_addr();
    unsigned long msr        = get_msr(IA32_APIC_BASE);

    /* Enable APIC if it's not enabled already */
    if ((msr & IA32_LAPIC_MSR_BASE) != lapic_addr || (msr & IA32_LAPIC_MSR_ENABLE) == 0) {
        msr = lapic_addr | IA32_LAPIC_MSR_ENABLE;
        set_msr(IA32_APIC_BASE, msr);
    }

    /* Because the Local APIC is above 2GB, we must explicitly map it to address space  */
    lapic_base = (uint8_t *)lapic_addr;
    mmu_map_page(lapic_addr, (unsigned long)lapic_base, MM_PRESENT | MM_READWRITE);

    write_32(lapic_base + LAPIC_REG_DFR, 0xffffffff);
    write_32(lapic_base + LAPIC_REG_TPR, 0);

    /* configure Local APIC Timer */
    __configure_timer();

    /* disable logical interrupt lines */
    write_32(lapic_base + LAPIC_REG_LINT0, LAPIC_INT_DISABLED_MASK);
    write_32(lapic_base + LAPIC_REG_LINT1, LAPIC_INT_DISABLED_MASK);

    /* enable Local APIC */
    write_32(lapic_base + LAPIC_REG_SVR, LAPIC_DM_SMI | LAPIC_SVR_ENABLE | VECNUM_SPURIOUS);

    /* acknowledge pending interrupts */
    write_32(lapic_base + LAPIC_REG_EOI, 0);

    isr_install_handler(VECNUM_TIMER,    __tmr_handler);
    isr_install_handler(VECNUM_SPURIOUS, __svr_handler);

    cpu_init_count++;
}

void lapic_register_dev(int cpu_id, int lapic_id)
{
    kassert(cpu_id >= 0 && cpu_id < MAX_CPU);

    lapics[cpu_id].cpu_id    = cpu_id;
    lapics[cpu_id].lapic_id = lapic_id;

    cpu_count++;
}

unsigned lapic_get_cpu_count(void)
{
    return cpu_count;
}

unsigned lapic_get_init_cpu_count(void)
{
    return cpu_init_count;
}

int lapic_get_lapic_id(unsigned cpu)
{
    if (cpu >= cpu_count)
        return -ENXIO;

    return lapics[cpu].lapic_id;
}

void lapic_send_ipi(uint32_t high, uint32_t low)
{
    write_32(lapic_base + LAPIC_REG_ICR_HI, high);
    write_32(lapic_base + LAPIC_REG_ICR_LO, low);
}

void lapic_send_init(unsigned cpu)
{
    uint32_t high = (lapics[cpu].lapic_id << 24) & 0xff000000;
    uint32_t low  = LAPIC_DM_INIT | LAPIC_TM_EDGE | LAPIC_LVL_ASSERT;

    lapic_send_ipi(high, low);
}

void lapic_send_sipi(unsigned cpu, unsigned vec)
{
    uint32_t high = (lapics[cpu].lapic_id << 24) & 0xff000000;
    uint32_t low  = (vec & 0xff) | LAPIC_DM_STARTUP | LAPIC_TM_EDGE | LAPIC_LVL_ASSERT;

    lapic_send_ipi(high, low);
}
