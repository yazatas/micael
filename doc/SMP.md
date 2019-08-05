# Symmetric Multiprocessing in micael

Support for SMP was added to micael in release Hyperion. Right now only x86_64 is supported.
ACPI and APIC must be supported by the target platform in order for SMP to work.

# SMP Boot

During the boot process, Boot Processor (BSP) initializes itself and the system (memory allocators, file system and the scheduler).
When everything has been initialized, the BSP starts the scheduler and jumps to init task.

Init task is responsible for waking up the Application Processors (APs) and it does so by first copying an SMP trampoline code (arch/x86_64/trampoline.S)
	to address 0x55000 (SIPI vector 0x55) and then sending INIT-SIPI-SIPI sequence to all APs. The AP receiving this sequence is woken up and is initially in 16-bit real mode. The whole purpose of the SMP trampoline is to switch from real mode to protected mode and jump to boot code.

When AP has reached the boot code, it will initialize itself just like BSP did during early boot and will eventually jump to `init_ap()` function. This function is responsible for fully initializing the AP and finally the `init_ap()` will call `sched_start()` which makes the AP to jump to idle_task.

When AP is ready, it will notify the BSP by incrementing `ap_initialized` variable in sched.c which makes BSP to send the INIT-SIPI-SIPI sequence to the next CPU. Finally when all APs have been initialized, the BSP will execute /sbin/init.

# Percpu variables

Percpu variables are an essential part of an SMP-compatible system. Each CPU has its own copy of the variable. 

In micael, percpu variables are defined as follows:

```
__percpu unsigned long counter = 0;
```

Percpu variables are used as follows:

```
/* Get local CPU variable or pointer */
int value = get_thiscpu_var(counter);

/* do something with counter */

put_thiscpu_var(counter);

/*******************************/

uint8_t *ptr = get_thiscpu_ptr(array);

/* do something with array */

put_thiscpu_var(array);
```

```
/* Variables of another cpu can be accessed like this */
int value = get_percpu_var(counter, 3);

/* do something with counter */

put_thiscpu_var(counter);

/*******************************/

uint8_t *ptr = get_percpu_ptr(array, 2);

/* do something with array */

put_percpu_var(array, 2);
```

When the system is booting, BSP will initialize the percpu areas for each CPU (ACPI has reported how many CPU's the system has). Each CPU will also do a "self-init" and during that it will save the percpu offset to GSBASE (register 0xC0000101). This value is used by `get_thiscpu_*` macros

Currently the `put_*` macros are not absolutely necessary because there's no kernel preemption. It is, however, a planned feature so it's wise to complement each `get_*` with `put_*` to reduce the amount of future work.


# SMP-awareness

Currently the SMP is supported by sprinkling spinlocks all over the place. There is, however, a need (and in fact a plan) to create dynamic percpu memory allocator. But this is a future task and now all CPUs must spin.

This is actually an interesting area of kernel developement and I will try to isolate the CPUs as much as I can to reduce the amount of waiting/locking.
