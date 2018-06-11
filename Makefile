ROOTDIR?=$(shell pwd)
include $(ROOTDIR)/Makefile.config

.PHONY: all kernel libc iso clean copy-headers

all: copy-headers libc kernel iso

copy-headers:
	@mkdir -p $(SYSROOT)
	$(MAKE) --directory=libc copy-headers
	$(MAKE) --directory=kernel copy-headers

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

debug:
	objcopy --only-keep-debug kernel/micael.kernel kernel.sym
	qemu-system-i386 -cdrom micael.iso $(QEMUFLAGS) -curses -gdb tcp::1337 -S

clean:
	$(MAKE) --directory=kernel clean
	$(MAKE) --directory=libc clean
	@rm -rf sysroot isodir micael.iso kernel.sym
