#include "fork.h"
#include "entry.h"
#include "mm.h"
#include "sched.h"

// Creates a task and adds it to the task array making it ready to run. Note
// that this function doesn't call schedule, so the task will be scheduled at a
// later time, but will not necessarily run immediately.
int copy_process(unsigned long clone_flags, unsigned long fn, unsigned long arg,
                 unsigned long stack) {
    // Not SMP safe
    preempt_disable();

    // We allocate a new page for the task. The task_struct goes at the bottom
    // (beginning) of the page and the stack goes at the top of the page
    // (growing down).
    struct task_struct *p = (struct task_struct *)get_free_page();
    if (!p) {
        return 1;
    }

    struct pt_regs *childregs = task_pt_regs(p);
    memzero((unsigned long)childregs, sizeof(struct pt_regs));
    memzero((unsigned long)&p->cpu_context, sizeof(struct cpu_context));

    if (clone_flags & PF_KTHREAD) {
        p->cpu_context.x19 = fn;
        p->cpu_context.x20 = arg;
    } else {
        struct pt_regs *cur_regs = task_pt_regs(current);
        // I think this is wrong in the book. I think the implementation of
        // their memcpy function is backwards and it "works" as a result.
        *childregs = *cur_regs;
        // x0 = 0 which is the return value of copy_process (0 for child, pid
        // for parent)
        childregs->regs[0] = 0;
        childregs->sp = stack + PAGE_SIZE;
        p->stack = stack;
    }

    p->flags = clone_flags;
    p->priority = current->priority;
    p->counter = p->priority;
    p->state = TASK_RUNNING;
    // This will be cleared in the sched_tail function (called by
    // ret_from_fork which is the first thing that will be executed when this
    // task runs for the first time).
    p->preempt_count = 1;

    // ret_from_fork will call schedule_tail and prepare to call fn (setting the
    // arguments to the register and jumping). Note that this task is currently
    // not running, we're running in the context of "current". This task starts
    // running here so that it can call preempt_disable on itself.
    p->cpu_context.pc = (unsigned long)ret_from_fork;
    // point the stack pointer after childregs is finished.
    p->cpu_context.sp = (unsigned long)childregs;

    // We could overflow here.
    int pid = nr_tasks++;
    task[pid] = p;

    preempt_enable();
    return pid;
}

int move_to_user_mode(unsigned long pc) {
    // pc is the address of the function that we're targeting once we move to
    // user mode.
    current->cpu_context.pc = pc;
    // Will be used on kernel_exit
    struct pt_regs *regs = task_pt_regs(current);
    memzero((unsigned long)regs, sizeof(*regs));

    // Pointer to the function that we want to execute
    regs->pc = pc;
    // Set the pstate to el0 so that when kernel_exit runs (eret), it will
    // return to user mode.
    regs->pstate = PSR_MODE_EL0t;
    unsigned long stack = get_free_page();
    if (!stack) {
        return -1;
    }
    // user space stack (not shared with the kernel).
    regs->sp = stack + PAGE_SIZE;
    current->stack = stack;
    return 0;
}

// Returns a section at the top of the page of where the task_struct is located
// where we store the pt_regs structure for that task.
struct pt_regs *task_pt_regs(struct task_struct *tsk) {
    // Not sure why THREAD_SIZE instead of PAGE_SIZE (which is what we're
    // allocating). Seems like THREAD_SIZE could become a problem if PAGE_SIZE
    // becomes too small for it.
    unsigned long p = (unsigned long)tsk + THREAD_SIZE - sizeof(struct pt_regs);
    return (struct pt_regs *)p;
}