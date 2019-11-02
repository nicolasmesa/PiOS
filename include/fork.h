#ifndef _FORK_H
#define _FORK_H

#include "sched.h"

// PSR bits
#define PSR_MODE_EL0t 0x00000000  // Make sure we return to EL0 level
#define PSR_MODE_EL1t 0x00000004
#define PSR_MODE_EL1h 0x00000005
#define PSR_MODE_EL2t 0x00000008
#define PSR_MODE_EL2h 0x00000009
#define PSR_MODE_EL3t 0x0000000c
#define PSR_MODE_EL3h 0x0000000d

int copy_process(unsigned long clone_flags, unsigned long fn,
                 unsigned long arg);
int move_to_user_mode(unsigned long start, unsigned long size,
                      unsigned long pc);
struct pt_regs *task_pt_regs(struct task_struct *tsk);

// Order here is very important! This needs to mimic the same way that
// kernel_entry/kernel_exit do their stuff.
struct pt_regs {
    unsigned long regs[31];

    // This gets popped in kernel_exit as x21 with the following instruction:
    // ldp	x30, x21, [sp, #16 * 15] // we ignore x30
    // We then assign x21 to sp_el0 (assuming we're returning to el0).
    unsigned long sp;

    // These two are popped in the kernel_exit macro as x22 and x23
    // with the following instruction:
    // ldp x22, x23, [sp, #16 * 16]
    // pc eventually gets assigned to elr_el1 and pstate to spsr_el1.
    unsigned long pc;
    unsigned long pstate;
};

#endif