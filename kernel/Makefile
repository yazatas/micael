ROOTDIR?=$(shell pwd)/..
include $(ROOTDIR)/Makefile.config
include $(ARCHDIR)/make.config
include $(DRIVERDIR)/make.config
include $(DIR_SCHED)/make.config
include $(DIR_SYNC)/make.config
include $(DIR_MM)/make.config
include $(DIR_FS)/make.config
include $(DIR_LIB)/make.config

DESTDIR=$(realpath $(shell pwd)/../sysroot)

CFLAGS  := $(CFLAGS) --sysroot=$(DESTDIR) $(KERNEL_ARCH_CFLAGS)
LDFLAGS := $(LDFLAGS) -nostdlib -lgcc $(KERNEL_ARCH_LDFLAGS)

KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
$(KERNEL_DRIVER_OBJS) \
$(KERNEL_SCHED_OBJS) \
$(KERNEL_SYNC_OBJS) \
$(KERNEL_MM_OBJS) \
$(KERNEL_FS_OBJS) \
$(KERNEL_LIB_OBJS) \
kernel/kmain.o \
kernel/isr.o \
kernel/kprint.o \
kernel/kpanic.o \
kernel/util.o

OBJS=$(KERNEL_OBJS)
LINK_LIST=$(KERNEL_OBJS) $(LDFLAGS)

.PHONY: all clean install copy-headers install-kernel
.SUFFIXES: .o .c .S

all: micael.kernel
install: copy-headers install-kernel

copy-headers:
	echo $(DESTDIR)
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: micael.kernel
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp micael.kernel $(DESTDIR)$(BOOTDIR)

micael.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
ifeq ($(AS), x86_64-elf-as)
	grub-file --is-x86-multiboot2 micael.kernel
else
	grub-file --is-x86-multiboot micael.kernel
endif

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS)

.S.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS)

clean:
	rm -f micael.kernel kernel.map
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d
