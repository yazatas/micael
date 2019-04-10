#ifndef __ARCH_X86_64_GDT_DEF_H__
#define __ARCH_X86_64_GDT_DEF_H__

#define GDT_ENTRY_SIZE 8

#define GDT_64BIT            (1 << 1)
#define GDT_32BIT_PROTECTED  (1 << 2)
#define GDT_4KB_GRANULARITY  (1 << 3)

#define GDT_PRESENT (1 << 7)
#define GDT_CPL0    (0 << 5)
#define GDT_CPL3    (3 << 5)
#define GDT_EXEC    (1 << 3)
#define GDT_EXEC_A  (1 << 3)
#define GDT_DD      (1 << 2)
#define GDT_RDWR    (1 << 1)
#define GDT_ACCESS  (1 << 0)

#define GDT_ENTRY(base, limit, flags, access)       \
    (                                               \
        (((((base)) >> 24) & 0xFF) << 56) |         \
        ((((flags)) & 0xF) << 52) |                 \
        (((((limit)) >> 16) & 0xF) << 48) |         \
        (((((access) | (1 << 4))) & 0xFF) << 40) |  \
        ((((base)) & 0xFFF) << 16) |                \
        (((limit)) & 0xFFFF)                        \
    )


#define GDT_ENTRY_NULL        GDT_ENTRY(0, 0,         0,                                   0)
#define GDT_ENTRY_KERNEL_CODE GDT_ENTRY(0, 0, GDT_64BIT, GDT_PRESENT | GDT_CPL0   | GDT_EXEC)

#define GDT_TABLE_ALIGNMENT 0x1000
#define GDT_TABLE_SIZE 0x100

#endif /* __ARCH_X86_64_GDT_DEF_H__ */
