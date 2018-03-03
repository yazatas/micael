# Memory management in micael

## Kernel memory layout

Kernel is loaded to physical address 0x00100000 and virtual address 0xc0100000.

| start address | reserved for |
| --------------| :------------|
| 0xc0000000    | kernel code, data, bss |
| 0xd0000000    | kernel heap |
| 0xe0000000    | free page frames for miscellaneous use |

