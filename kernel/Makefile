ROOTDIR?=$(shell pwd)/..
include $(ROOTDIR)/Makefile.config
include $(ARCHDIR)/make.config
include $(DRIVERDIR)/make.config
include $(ALGODIR)/make.config
include $(SCHEDDIR)/make.config
include $(DIR_SYNC)/make.config
include $(DIR_MM)/make.config

DESTDIR=$(realpath $(shell pwd)/../sysroot)

CFLAGS:=$(CFLAGS) --sysroot=$(DESTDIR) -D__is_kernel $(KERNEL_ARCH_CFLAGS)
LDFLAGS:=$(LDFLAGS) -nostdlib -lk -lgcc $(KERNEL_ARCH_LDFLAGS)


KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
$(KERNEL_DRIVER_OBJS) \
$(KERNEL_ALGO_OBJS) \
$(KERNEL_SCHED_OBJS) \
$(KERNEL_SYNC_OBJS) \
$(KERNEL_MM_OBJS) \
kernel/kmain.o \
kernel/isr.o \
kernel/kprint.o \
kernel/kpanic.o \
kernel/task.o \
kernel/mutex.o \

OBJS=$(KERNEL_OBJS)
LINK_LIST=$(KERNEL_OBJS) $(LDFLAGS)

.PHONY: all clean install install-headers install-kernel
.SUFFIXES: .o .c .S

all: micael.kernel
install: install-headers install-kernel

install-headers:
	echo $(DESTDIR)
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: micael.kernel
	echo $(DESTDIR)
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp micael.kernel $(DESTDIR)$(BOOTDIR)

micael.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	grub-file --is-x86-multiboot micael.kernel

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS)

.S.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS)

clean:
	rm -f micael.kernel kernel.map
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d
