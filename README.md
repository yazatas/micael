# micael

micael is a 32-bit monolithic kernel written in C and x86 assembly.

Currently micael is in very unstable state and has pretty much no functionality. I'm currently in the process of implementing libc which takes a lot of time. Only a few of the functions in libc/\* have been tested so there are a lot of bugs until I have time to properly test them all.

You'll need a [cross-compiler](http://wiki.osdev.org/GCC_Cross-Compiler) to compile micael on your system (don't worry, it's easy to build). In case you just want to run micael, you'll need QEMU.

# Compiling and running

Run following commands to compile and run micael:

`./build.sh; ./run`

# Copying
micael is free software. It's licensed under the MIT license.
