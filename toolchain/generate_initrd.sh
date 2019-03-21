#!/bin/bash

gcc -o mkinitrd mkinitrd.c

cd programs/src
./make_program.sh init.c  ../bin/init.bin
./make_program.sh shell.c ../bin/shell.bin
./make_program.sh echo.c  ../bin/echo.bin
./make_program.sh calculator.c ../bin/calculator.bin

cd ../..
./mkinitrd \
	-f programs/bin/init.bin "/sbin/init"\
   	-f programs/bin/shell.bin "/sbin/dsh"\
   	-f programs/bin/echo.bin "/bin/echo"\
	-f programs/bin/calculator.bin "/bin/calc"
