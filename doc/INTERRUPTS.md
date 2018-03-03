# Interrupts in micael

micael has both software and hardware interrupts. For now only kernel uses interrupts but in the future user mode programs can raise interrupts (indirectly) by issuing system calls.

ISRs 0-16 are reserved for OS (division by zero, page fault, double fault etc.), 17-31 are reserved for the processor and interrupts from 32 to 47 are used for interrupt requests (PIT, keyboard etc).

micael's interrupt handler is very simple and right now only handles page faults. If any other fault is raised, kernel panic occurs and system must be rebooted. When user mode support is added the interrupt handler will be improved significantly. But for now handling page faults is enough.

# system calls

TODO
