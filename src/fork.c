#include "entry.h"
#include "mm.h"
#include "sched.h"

// Creates a task and adds it to the task array making it ready to run. Note
// that this function doesn't call schedule, so the task will be scheduled at a
// later time, but will not necessarily run immediately.
int copy_process(unsigned long fn, unsigned long arg) {
    // Not SMP safe
    preempt_disable();

    // We allocate a new page for the task. The task_struct goes at the bottom
    // (beginning) of the page and the stack goes at the top of the page
    // (growing down).
    struct task_struct *p = (struct task_struct *)get_free_page();
    if (!p) {
        return 1;
    }

    p->priority = current->priority;
    p->counter = p->priority;
    p->state = TASK_RUNNING;
    // This will be cleared in the sched_tail function (called by
    // ret_from_fork which is the first thing that will be executed when this
    // task runs for the first time).
    p->preempt_count = 1;

    p->cpu_context.x19 = fn;
    p->cpu_context.x20 = arg;
    // ret_from_fork will call schedule_tail and prepare to call fn (setting the
    // arguments to the register and jumping). Note that this task is currently
    // not running, we're running in the context of "current". This task starts
    // running here so that it can call preempt_disable on itself.
    p->cpu_context.pc = (unsigned long)ret_from_fork;
    // point the stack pointer to the top of the page
    p->cpu_context.sp = (unsigned long)p + THREAD_SIZE;

    // We could overflow here.
    int pid = nr_tasks++;
    task[pid] = p;

    preempt_enable();
    return 0;
}