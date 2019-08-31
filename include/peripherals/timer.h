#ifndef _P_TIMER_H
#define _P_TIMER_H

#include "peripherals/base.h"

// Used to acknowledge that a timer interrupt was handled. This is done by
// setting The specific timer bit (see to TIMER_CS_M1).
#define TIMER_CS (PBASE + 0x00003000)

// Timer counter (incremented by 1 every tick)
#define TIMER_CLO (PBASE + 0x00003004)
#define TIMER_CHI (PBASE + 0x00003008)
#define TIMER_C0 (PBASE + 0x0000300C)
#define TIMER_C1 (PBASE + 0x00003010)
#define TIMER_C2 (PBASE + 0x00003014)
#define TIMER_C3 (PBASE + 0x00003018)

#define TIMER_CS_M0 (1 << 0)
#define TIMER_CS_M1 (1 << 1)
#define TIMER_CS_M2 (1 << 2)
#define TIMER_CS_M3 (1 << 3)

#endif /*_P_TIMER_H */