ROOTDIR?=$(shell pwd)
include $(ROOTDIR)/Makefile.config

.PHONY: all kernel libc iso clean install-headers

all: install-headers libc kernel iso

install-headers:
	@mkdir -p $(SYSROOT)
	$(MAKE) --directory=libc install-headers
	$(MAKE) --directory=kernel install-headers

kernel:
	$(MAKE) --directory=kernel install

libc:
	$(MAKE) --directory=libc install

iso:
	@mkdir -p isodir/boot/grub
	@cp sysroot/boot/micael.kernel isodir/boot/micael.kernel
	@echo "menuentry "micael" { multiboot /boot/micael.kernel }" > isodir/boot/grub/grub.cfg
	@grub-mkrescue -o micael.iso isodir

run:
	qemu-system-i386 -cdrom micael.iso $(QEMUFLAGS) &> /dev/null

clean:
	$(MAKE) --directory=kernel clean
	$(MAKE) --directory=libc clean
	@rm -rf sysroot isodir micael.iso
