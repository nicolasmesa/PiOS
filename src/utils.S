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


.global set_pgd
set_pgd:
    msr ttbr0_el1, x0
    // From http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0488d/CIHDFEHE.html
    // Invalidate all stage 1 translations used at EL1 with the current virtual machine
    // identifier (VMID) in the Inner Shareable.
    tlbi vmalle1is
    // Ensure completion of the TLB invalidation
    DSB ISH
    // From http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0489c/CIHGHHIE.html
    // Instruction Synchronization Barrier.
    // Flushes the pipeline in the processor, so that all instructions following the ISB are
    // fetched from cache or memory, after the instruction has been completed. It ensures
    // that the effects of context altering operations, such as changing the ASID, or
    // completed TLB maintenance operations, or branch predictor maintenance operations, as
    // well as all changes to the CP15 registers, executed before the ISB instruction are
    // visible to the instructions fetched after the ISB.
    isb
    ret

.global get_pgd
get_pgd:
    mov x1, 0
    ldr x0, [x1]
    mov x0, 0x1000
    msr ttbr0_el1, x0
    ldr x0, [x1]
    ret