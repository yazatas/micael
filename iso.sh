#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/micael.kernel isodir/boot/micael.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "micael" {
	multiboot /boot/micael.kernel
}
EOF
grub-mkrescue -o micael.iso isodir
