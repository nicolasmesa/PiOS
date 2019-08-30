#include "mm.h"
#include "arm/sysregs.h"

.section ".text.boot"

.global _start
_start:
    mrs x0, mpidr_el1
    and x0, x0,#0xFF // We do this and to strip the last byte of the value coming from mpidr_el1
    cbz x0, master
    b  proc_hang

proc_hang:
    b proc_hang


.global master
master:
    // To allow UART boot to keep working, we first check what's the current Exception level. If
    // we're already in #1, it means that there was a kernel here before that loaded us into #1
    // so we skip this part.
    mrs x0, CurrentEL
    lsr x0, x0, #2
    cmp x0, #1
    beq el1_entry

    ldr x0, =SCTLR_VALUE_MMU_DISABLED
    msr sctlr_el1, x0

    ldr x0, =HCR_VALUE
    msr hcr_el2, x0
    ldr x0, =SCR_VALUE
    msr scr_el3, x0

    ldr x0, =SPSR_VALUE
    msr spsr_el3, x0

    adr x0, el1_entry
    msr elr_el3, x0

    eret

el1_entry:
    adr x0, bss_begin
    adr x1, bss_end
    sub x1, x1, x0
    bl  memzero // Branch with a link (when the function call is done, it should come back here)
setup_stack:
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    mov x1, #STACK_SIZE
    mul x0, x0, x1 // x0 now holds the offset to use (0, 16KB, 32KB, 48KB) depending on the cpuid
    mov x1, #LOW_MEMORY
    sub x0, x1, x0 // x1 holds LOW_MEMORY. Here, we subtract from LOW_MEMORY (we're setting up our stacks downward).
    mov sp, x0
    bl kernel_main