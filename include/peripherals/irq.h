#ifndef _P_IRQ_H
#define _P_IRQ_H

#include "peripherals/base.h"

#define IRQ_BASIC_PENDING (PBASE + 0x0000B200)

// These two locations are related with the ENABLE_IRQS_n locations below. These
// hold the type of interrupt that was triggered (a bit of 1). Note that
// multiple interrupts can be active at any given time.
#define IRQ_PENDING_1 (PBASE + 0x0000B204)
#define IRQ_PENDING_2 (PBASE + 0x0000B208)

#define FIQ_CONTROL (PBASE + 0x0000B20C)

// Bits 0-31 control the IRQs from IRQ 0 - 31 (ENABLE_IRQS_2 controls the rest).
// Writing a 1 to a bit will set the corresponding IRQ enable bit. All other IRQ
// enable bits are unaffected.
// References:
//  BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf (page 116)
//  https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson03/rpi-os.md#configuring-interrupt-controller
#define ENABLE_IRQS_1 (PBASE + 0x0000B210)
#define ENABLE_IRQS_2 (PBASE + 0x0000B214)

#define ENABLE_BASIC_IRQS (PBASE + 0x0000B218)
#define DISABLE_IRQS_1 (PBASE + 0x0000B21C)
#define DISABLE_IRQS_2 (PBASE + 0x0000B220)
#define DISABLE_BASIC_IRQS (PBASE + 0x0000B224)

#define SYSTEM_TIMER_IRQ_0 (1 << 0)  // Reserved and used by the GPU
#define SYSTEM_TIMER_IRQ_1 (1 << 1)  // We use this one
#define SYSTEM_TIMER_IRQ_2 (1 << 2)  // Reserved and used by the GPU
#define SYSTEM_TIMER_IRQ_3 (1 << 3)

#endif /*_P_IRQ_H */