#include "peripherals/timer.h"
#include "printf.h"
#include "sched.h"
#include "utils.h"

// This constant defines how many ticks we want between timer interrupts.
const unsigned int interval = 200000;

// Not SMP safe. Maybe having an array with the number of CPUs and modifying in
// with the CPU number could solve it.
unsigned int curVal = 0;

// The timer is a counter (TIMER_CLO) that increases by 1 every tick. We set a
// compare register (Compare Register 1 in this case) that the timer compares
// agains every tick. If the comparison matches, it generates an interrupt.
void timer_init(void) {
    curVal = get32(TIMER_CLO);
    curVal += interval;
    put32(TIMER_C1, curVal);
}

void handle_timer_irq(void) {
    curVal += interval;

    // Update the TIMER_C1 to get interrupted again after interval ticks
    put32(TIMER_C1, curVal);

    // We set this bit (2nd bit) in the TIMER_CS register to indicate that we
    // handled that timer interrupt
    put32(TIMER_CS, TIMER_CS_M1);

    // Notify scheduler of tick
    timer_tick();
}