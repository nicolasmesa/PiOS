#include "arm/sysregs.h"
#include "entry.h"
#include "sys.h"

// Start macros

// Macro that prepares arguments to call the show_invalid_entry_message function
// and hangs
.macro handle_invalid_entry el, type
    kernel_entry \el
    mov x0, #\type
    mrs x1, esr_el1 // Exception Syndrome Register: Info about what caused the exception.
    mrs x2, elr_el1 // Exception Link Register: Instruction that caused the exception.
    bl show_invalid_entry_message
    b err_hang
.endm

// Macro that generates an entry in the interrupt vector. All it does is jump to the 
// specified label. It aligns to 7 because all instructions need to be 0x80 (128) bytes
// from one another.
.macro ventry label
.align 7
    b \label
.endm

// Macro executed whenever we enter kernel space. It simply saves all registers onto the stack. 
// Note that exceptions are masked at this point to give us time to save state.
// Receives an arg "el" which indicates if the we're entering from el0 or el1.
.macro kernel_entry el
    sub sp, sp, #S_FRAME_SIZE
    stp x0, x1, [sp, #16 * 0] // stp: Store pair of registers (each register is 8 bytes to we offest by #16)
    stp x2, x3, [sp, #16 * 1]
    stp x4, x5, [sp, #16 * 2]
    stp x6, x7, [sp, #16 * 3]
    stp x8, x9, [sp, #16 * 4]
    stp x10, x11, [sp, #16 * 5]
    stp x12, x13, [sp, #16 * 6]
    stp x14, x15, [sp, #16 * 7]
    stp x16, x17, [sp, #16 * 8]
    stp x18, x19, [sp, #16 * 9]
    stp x20, x21, [sp, #16 * 10]
    stp x22, x23, [sp, #16 * 11]
    stp x24, x25, [sp, #16 * 12]
    stp x26, x27, [sp, #16 * 13]
    stp x28, x29, [sp, #16 * 14]

    // Here, we're saving the current stack pointer from el0. We basically have two stack pointers, one for user space
    // (allocated by the user before calling clone() or by the kernel in move_to_user_mode), and one by the kernel
    // (which lives in the same page as the task_struct). We need to make sure that we save the current stack pointer.
    // Note that sp currently points to the top of the stack in ther kernel (we're running in el1 right now).
    .if \el == 0
    mrs x21, sp_el0
    .else
    add x21, sp, #S_FRAME_SIZE
    .endif /* \el == 0 */

    // We need to store these because, during an interrupt, we might come in as one task but finish execution
    // as a different task. We want to make sure that the task this task continues running where it left off
    // once it returns. Note that the stack changes from task to task, so whenever this task returns, it will
    // have these values on the stack.
    mrs	x22, elr_el1
    mrs	x23, spsr_el1

    stp x30, x21, [sp, #16 * 15]
    stp x22, x23, [sp, #16 * 16] 
.endm

// Macro executed when we exit the kernel back to the previous execution state. We "pop" all registers
// back from the stack and set the stack pointer back to where it was. Note that interrupts are masked
// at this point and will be unmasked after we eret.
// Receives an arg "el" which indicates if the return is to el0 or el1.
.macro kernel_exit, el
    // restore the registers needed to return from the interrupt first.
    ldp	x22, x23, [sp, #16 * 16]
    ldp	x30, x21, [sp, #16 * 15] 

    // Here, sp points to the top of the stack in the kernel side for this task (the page with task_struct).
    // We only need to restore sp in eret if we're returning to el0. If we're not returning to el0, we're
    // already pointing to the correct stack.
    .if \el == 0
    msr sp_el0, x21
    .endif /* \el == 0 */

    // Pointer to where we return to (Exception Link Register)
    msr	elr_el1, x22
    // Saved Program Status Register (processor state). This ensures that we return to the right exception level
    // on eret
    msr	spsr_el1, x23

    ldp x0, x1, [sp, #16 * 0] // ldp: Load pair of registers.
    ldp x2, x3, [sp, #16 * 1] 
    ldp x4, x5, [sp, #16 * 2]
    ldp x6, x7, [sp, #16 * 3]
    ldp x8, x9, [sp, #16 * 4]
    ldp x10, x11, [sp, #16 * 5]
    ldp x12, x13, [sp, #16 * 6]
    ldp x14, x15, [sp, #16 * 7]
    ldp x16, x17, [sp, #16 * 8]
    ldp x18, x19, [sp, #16 * 9]
    ldp x20, x21, [sp, #16 * 10]
    ldp x22, x23, [sp, #16 * 11]
    ldp x24, x25, [sp, #16 * 12]
    ldp x26, x27, [sp, #16 * 13]
    ldp x28, x29, [sp, #16 * 14]
    add sp, sp, #S_FRAME_SIZE  
    eret
.endm


/*
 * Exception vectors.
 */
.align 11
.globl vectors 
vectors:
    // Expanded, looks something like:
    // .align 7
    // b sync_invalid_el1t 
    // .align 7
    // b irq_invalid_el1t
    // ...
    ventry sync_invalid_el1t   // Synchronous EL1t
    ventry irq_invalid_el1t   // IRQ EL1t
    ventry fiq_invalid_el1t   // FIQ EL1t
    ventry error_invalid_el1t   // Error EL1t

    ventry sync_invalid_el1h   // Synchronous EL1h
    ventry el1_irq     // IRQ EL1h
    ventry fiq_invalid_el1h   // FIQ EL1h
    ventry error_invalid_el1h   // Error EL1h

    ventry el0_sync   // Synchronous 64-bit EL0
    ventry el0_irq // IRQ 64-bit EL0
    ventry fiq_invalid_el0_64   // FIQ 64-bit EL0
    ventry error_invalid_el0_64   // Error 64-bit EL0

    ventry sync_invalid_el0_32   // Synchronous 32-bit EL0
    ventry irq_invalid_el0_32   // IRQ 32-bit EL0
    ventry fiq_invalid_el0_32   // FIQ 32-bit EL0
    ventry error_invalid_el0_32   // Error 32-bit EL0

sync_invalid_el1t:
    // Expanded looks somethng like:
    // kernel_entry 0// Not expanded
    // mov x0, #SYNC_INVALID_EL1t
    // mrs x1, esr_el1
    // mrs x2, elr_el1
    // bl show_invalid_entry_message
    // b err_hang 
    handle_invalid_entry 1, SYNC_INVALID_EL1t

irq_invalid_el1t:
    handle_invalid_entry  1, IRQ_INVALID_EL1t

fiq_invalid_el1t:
    handle_invalid_entry  1, FIQ_INVALID_EL1t

error_invalid_el1t:
    handle_invalid_entry  1, ERROR_INVALID_EL1t

sync_invalid_el1h:
    handle_invalid_entry  1, SYNC_INVALID_EL1h

fiq_invalid_el1h:
    handle_invalid_entry  1, FIQ_INVALID_EL1h

error_invalid_el1h:
    handle_invalid_entry  1, ERROR_INVALID_EL1h

fiq_invalid_el0_64:
    handle_invalid_entry  0, FIQ_INVALID_EL0_64

error_invalid_el0_64:
    handle_invalid_entry  0, ERROR_INVALID_EL0_64

sync_invalid_el0_32:
    handle_invalid_entry  0, SYNC_INVALID_EL0_32

irq_invalid_el0_32:
    handle_invalid_entry  0, IRQ_INVALID_EL0_32

fiq_invalid_el0_32:
    handle_invalid_entry  0, FIQ_INVALID_EL0_32

error_invalid_el0_32:
    handle_invalid_entry  0, ERROR_INVALID_EL0_32

// Handles interrupt requests
el1_irq:
    kernel_entry 1
    bl handle_irq
    kernel_exit 1

el0_irq:
    kernel_entry 0 
    bl	handle_irq
    kernel_exit 0 

el0_sync:
    kernel_entry 0
    mrs x25, esr_el1
    lsr x24, x25, #ESR_ELx_EC_SHIFT // get the exception field
    // compare exception field to see if it was caused by the svc instruction.
    // Other possible things can be, for example, accessing a register that is not available in el0.
    cmp x24, #ESR_ELx_EC_SVC64
    b.eq el0_svc // branch to our svc handler for el0

    cmp x24, #ESR_ELx_EC_DABT_LOW // page fault synchronous exception. (or data access exception)
    b.eq el0_da

    handle_invalid_entry 0, SYNC_ERROR

// Here we're simply creating aliases for the registers
// sc_nr == x25
sc_nr .req x25 // number of system calls
scno .req x26 // syscall number
stbl .req x27 // syscall table pointer

// Handle a software interrupt (Supervisor call)
el0_svc:
    adr stbl, sys_call_table // not sure where this pointer comes from
    uxtw scno, w8 // not sure what uxtw does
    mov sc_nr, #__NR_syscalls
    bl enable_irq

    // compare the syscall number to the number of syscalls and call ni_sys if it's greater-than or equal.
    cmp scno, sc_nr
    b.hs ni_sys

    ldr x16, [stbl, scno, lsl #3] // x6 = stbl[scno << 3] = stbl[scno * 8] (pointers are 8 bytes)
    blr x16 // branch to register
    b ret_from_syscall

// Handle a page fault
el0_da:
    bl enable_irq
    // far_el1 is the fault address register and contains the address that the process tried to access
    // and triggered the page fault.
    mrs x0, far_el1
    mrs x1, esr_el1 // exception syndrom register.
    bl do_mem_abort
    cmp x0, 0
    b.eq 1f
    handle_invalid_entry 0, DATA_ABORT_ERROR
1: bl disable_irq
   kernel_exit 0


// This hangs the CPU, so there's no need to disable_irq. We could return an
// -1 or whatever instead of hanging the CPU because of a bad syscall.
ni_sys:
    handle_invalid_entry 0, SYSCALL_ERROR

ret_from_syscall:
    bl disable_irq
    // We place x0 in sp[#S_X0] = sp[0] = x0. kernel exit will take care of
    // popping this off the stack to return the syscall return value.
    // Note that sp[0] points to the value of x0 saved on the stack because kernel_entry
    // does "sub sp, sp, #S_FRAME_SIZE" and then saves x0 at the "top" of the stack (grows downward)
    // by doing "stp x0, x1, [sp, 16 * 0]"
    str x0, [sp, #S_X0]
    kernel_exit 0

.globl ret_from_fork
ret_from_fork:
    bl	schedule_tail
    // I assume x19 decides if we're a user process or not.
    cbz x19, ret_to_user
    // x20 holds the argument to the function, we move it to x0.
    mov	x0, x20
    // x19 holds the address of the function to call.
    // If this branch returns, it means that the kernel process was preparing everything
    // to move to user process, so we do just that (we fall through to ret_to_user).
    blr	x19 
// Since every forked kernel process starts its life in the ret_from_fork function, the stack
// at this point is completely empty (from the process perspective). However, the pt_regs are still
// there, which means that kernel_exit will restore them when it runs.
ret_to_user:
    bl disable_irq
    kernel_exit 0 // exit to user mode (el0).


.globl err_hang
err_hang:
    b err_hang