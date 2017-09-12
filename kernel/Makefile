ROOTDIR?=$(shell pwd)/..
include $(ROOTDIR)/Makefile.config
include $(ARCHDIR)/make.config

DESTDIR=$(realpath $(shell pwd)/../sysroot)

CFLAGS:=$(CFLAGS) --sysroot=$(DESTDIR) -D__is_kernel $(KERNEL_ARCH_CFLAGS)
LDFLAGS:=$(LDFLAGS) -nostdlib -lk -lgcc $(KERNEL_ARCH_LDFLAGS)


KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
kernel/kernel.o \
kernel/isr.o \
kernel/timer.o \
kernel/keyboard.o \
kernel/kprint.o \
kernel/kpanic.o \
kernel/kheap.o  \

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
	rm -f micael.kernel
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d
