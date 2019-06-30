ROOTDIR?=$(shell pwd)
include $(ROOTDIR)/Makefile.config

.PHONY: all kernel iso clean copy-headers toolchain

all: copy-headers kernel toolchain iso

copy-headers:
	@mkdir -p $(SYSROOT)
	$(MAKE) --directory=kernel copy-headers

kernel:
	$(MAKE) --directory=kernel install

toolchain:
	$(MAKE) --directory=toolchain all

iso:
	@mkdir -p isodir/boot/grub
	@cp toolchain/initrd.bin isodir/boot
	@cp sysroot/boot/micael.kernel isodir/boot/micael.kernel
	@echo "set timeout=0"                   > isodir/boot/grub/grub.cfg
	@echo "set default=0"                  >> isodir/boot/grub/grub.cfg
	@echo "menuentry micael {"              >> isodir/boot/grub/grub.cfg
	@echo "multiboot2 /boot/micael.kernel}" >> isodir/boot/grub/grub.cfg
	#@echo "module /boot/initrd.bin }"     >> isodir/boot/grub/grub.cfg
	@grub-mkrescue -o micael.iso isodir

run:
	qemu-system-x86_64 -cdrom micael.iso $(QEMUFLAGS) &> /dev/null

debug:
	objcopy --only-keep-debug kernel/micael.kernel kernel.sym
	qemu-system-x86_64 -cdrom micael.iso $(QEMUFLAGS) -gdb tcp::1337 -S

clean:
	$(MAKE) --directory=kernel clean
	$(MAKE) --directory=toolchain clean
	@rm -rf sysroot isodir micael.iso kernel.sym
