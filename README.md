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

## Multiprocessor

I added minimal multiprocessor support to the kernel and it even works when you send it over UART. The code is not very elegant
(it's actually quite hacky) but it works. Since the original tutorial doesn't include multiprocessor support, I created a
[branch](https://github.com/nicolasmesa/PiOS/tree/multiprocessor-booting) with the code to support it and removed the
multiprocessor support from master. One day, I might try to port other functionality to make it work with multiple CPUs.

### Sending over UART

The instructions to send the kernel over UART for multiprocessor support is the same as described above. The only difference is
that you must first copy the kernel that supports multiple processors to the SDCard. Then, you'll be able to send your kernel over.

#### Commands

#### Install multiprocessor kernel

```
$ git checkout multiprocessor-booting
$ ./build.sh
$ cp kernel8.img /Volumes/boot/
```
