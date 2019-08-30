#include "mm.h"

.global put32
put32:
    str w1,[x0]
    ret

.global get32
get32:
    ldr w0, [x0]
    ret

.global delay
delay:
    subs x0, x0, #1
    bne delay
    ret

.global branch_to_address
branch_to_address:
    br x0
    ret // We never reach this point

.global get_cpuid
get_cpuid:
    mrs x0, mpidr_el1
    and x0, x0,#0xFF
    ret

// Get current Exception level (0-3 with 0 being the least privileged)
.global get_el
get_el:
    mrs x0, CurrentEL
    lsr x0, x0, #2
    ret