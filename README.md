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
