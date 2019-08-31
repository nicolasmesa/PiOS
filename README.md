# RPi OS

Project to create a toy OS for the raspberry pi based on [this tutorial](https://github.com/s-matyukevich/raspberry-pi-os)

## Building / installing the kernel

To build the kernel, simply run the `build.sh` script. Then, you can copy the kernel image to the `boot` partition of the
sdcard of the RPI.

```
$ build.sh
$ cp kernel8.img /Volumes/boot/
```

## Sending the kernel over UART

Having to use the sdcard every time makes the kernel development a lot more cumbersome. You can send the kernel over UART.
To do this, you'll be required to copy the kernel to the sdcard (see previous section) only once. Then, you'll be able to
use the `boot_send.py` script to send the kernel and start an interactive session over UART.

For a detailed explanation of how this works, check out my blog post
[Booting Your Own Kernel on Raspberry Pi via Uart](https://blog.nicolasmesa.co/posts/2019/08/booting-your-own-kernel-on-raspberry-pi-via-uart/).

### Prerequisites

Make sure you're running in a virtual environment and install the requirements (I'm using python3.7)

```
$ source activate rpi_os
(rpi_os) $ pip install -r requirements.txt
```

### Running the script

#### Sending the kernel

```
(rpi_os) $ python boot_send.py -d /dev/cu.SLAB_USBtoUART -b 115200 -k ../kernel8.img
Sending kernel with size 1887 and checksum 187912
Kernel size confirmed. Sending kernel
Validating checksum...
Sending kernel with size 1863 and checksum 185855
Kernel size confirmed. Sending kernel
Validating checksum...
Received:  Done copying kernel
```

#### Starting an interactive session

```
(rpi_os) $ python boot_send.py -d /dev/cu.SLAB_USBtoUART -b 115200 -i
Making it interactive. You may need to press enter...
Hello world!
```

At this point, you're running in your brand new kernel. However, it is stuck waiting for a line. If you type `kernel` and press
enter, you'll get into the send kernel workflow again (not what you want). Press enter to skip that workflow and boot into your
newly compiled kernel. Note that this kernel doesn't persist after a reboot (it lives solely in memory).

#### Doing both at the same time

```
(rpi_os) $ python boot_send.py -d /dev/cu.SLAB_USBtoUART -b 115200 -k ../kernel8.img -i
Sending kernel with size 1863 and checksum 185855
Kernel size confirmed. Sending kernel
Validating checksum...
Received:  Done copying kernel

Making it interactive. You may need to press enter...
Hello world!
```

#### Debugging

Sometimes the uart_boot send will act up. This usually happens becaue the kernel in the SD card has a different function layout and
CPUs 1-3 start going haywire (if you modify the `hang` function for example). I added a `--debug` flag that will make the kernel
echo every byte we send and the `boot_send` script will verify each byte before moving to the next one. This will allow you to
tell which byte is failing which will help you understand what is getting overwritten.

```
(rpi_os) $ python boot_send.py -d /dev/cu.SLAB_USBtoUART -b 115200 -k ../kernel8.img --debug
Sending kernel with size 12376 and checksum 843818
Kernel size confirmed. Sending kernel
Starting debug workflow
0 Sending byte 160
0 Received byte 160
1 Sending byte 0
1 Received byte 0
2 Sending byte 56
2 Received byte 56
3 Sending byte 213
3 Received byte 213
...
```

## Multiprocessor

I added minimal multiprocessor support to the kernel and it even works when you send it over UART. The code is not very elegant
(it's actually quite hacky) but it works. Since the original tutorial doesn't include multiprocessor support, I created a
[branch](https://github.com/nicolasmesa/PiOS/tree/multiprocessor-booting) with the code to support it and removed the
multiprocessor support from master. One day, I might try to port other functionality to make it work with multiple CPUs.

### Sending over UART

The instructions to send the kernel over UART for multiprocessor support is the same as described above. The only difference is
that you must first copy the kernel that supports multiple processors to the SDCard. Then, you'll be able to send your kernel over.

## Debugging

### objdump

`objdump` is a good place to start looking at the binary. From the root directory, run:

```
$ objdump -d build/kernel8.elf
build/kernel8.elf:      file format ELF64-aarch64-little

Disassembly of section .text.boot:
_start:
       0:       a0 00 38 d5     mrs     x0, MPIDR_EL1
       4:       00 1c 40 92     and     x0, x0, #0xff
       8:       60 00 00 b4     cbz     x0, #12
       c:       01 00 00 14     b       #4

proc_hang:
      10:       00 00 00 14     b       #0
...
```

#### Commands

#### Install multiprocessor kernel

```
$ git checkout multiprocessor-booting
$ ./build.sh
$ cp kernel8.img /Volumes/boot/
```
