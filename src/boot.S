#include "mm.h"

.section ".text.boot"

.global _start
_start:
    mrs x0, mpidr_el1
    and x0, x0,#0xFF // We do this and to strip the last byte of the value coming from mpidr_el1
    cbz x0, master
    b  setup_stack 

proc_hang:
    b proc_hang

master:
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