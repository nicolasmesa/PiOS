#ifndef _P_BASE_H
#define _P_BASE_H

#include "mm.h"

// The RPi reserves memory above this address for devices
// See:
// https://github.com/raspberrypi/documentation/files/1888662/BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf
#define DEVICE_BASE 0x3F000000

// We add VA_START because the peripheral access will also be addressed via
// virtual memory.
#define PBASE (VA_START + DEVICE_BASE)

#endif /*_P_BASE_H */
