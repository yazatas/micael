.PHONY: all programs clean

DEPS = programs/bin/init.o \
	   programs/bin/shell.o \
	   programs/bin/echo.o \
	   programs/bin/calculator.o

all: programs mkinitrd

programs:
	$(MAKE) --directory=programs all

mkinitrd: $(DEPS)
	gcc -o mkinitrd util/mkinitrd.c
	./mkinitrd \
		-f programs/bin/init.o "/sbin/init" \
		-f programs/bin/shell.o "/sbin/dsh" \
		-f programs/bin/echo.o "/bin/echo" \
		-f programs/bin/calculator.o "/bin/calc" \
		-f programs/bin/spawn.o "/bin/spawn"

clean:
	$(MAKE) --directory=programs clean
	rm -f initrd.bin mkinitrd
