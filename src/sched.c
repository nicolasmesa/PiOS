#include "sched.h"
#include "irq.h"
#include "printf.h"

static struct task_struct init_task = INIT_TASK;

// Points to the task that is currently executing (not SMP safe)
struct task_struct *current = &(init_task);

// Holds references to all the tasks available.
struct task_struct *task[NR_TASKS] = {
    &(init_task),
};

// Number of currently running tasks in the system.
int nr_tasks = 1;

void preempt_disable(void) { current->preempt_count++; }

void preempt_enable(void) { current->preempt_count--; }

// This function is executed when a task is executing for the first time. We
// make sure to call preempt_enable so that it can be preempted going forward.
void schedule_tail(void) { preempt_enable(); }

void switch_to(struct task_struct *next) {
    if (current == next) {
        return;
    }

    struct task_struct *prev = current;
    current = next;
    cpu_switch_to(prev, current);
}

void _schedule(void) {
    preempt_disable();

    int next, c;
    struct task_struct *p;

    while (1) {
        c = -1;

        // Find runnable task that has the highest counter.
        for (int i = 0; i < NR_TASKS; i++) {
            p = task[i];
            if (p && p->state == TASK_RUNNING && p->counter > c) {
                c = p->counter;
                next = i;
            }
        }

        // If we found a task, we schedule that one.
        if (c > 0) {
            break;
        }

        // If we end up here, it means that either, there are no runnable tasks,
        // or all tasks have counter <= 0. Here we go through all the tasks and
        // update their counters. If none of the tasks are runnable, the outer
        // while loop will keep executing until one of them becomes runnable
        // (once an interrupt unblocks a task).
        for (int i = 0; i < NR_TASKS; i++) {
            p = task[i];
            if (p) {
                p->counter = (p->counter >> 1) + p->priority;
            }
        }
    }

    switch_to(task[next]);

    preempt_enable();
}

void timer_tick(void) {
    --current->counter;
    if (current->counter > 0 || current->preempt_count > 0) {
        return;
    }

    current->counter = 0;

    // timer_tick is called with interrupts disabled. We reenable them while we
    // call _schedule and then disable them again.
    enable_irq();
    _schedule();
    disable_irq();
}

void schedule(void) {
    // Set counter to 0 to prevent the same task from being scheduled again
    // since it is giving up the CPU voluntarily.
    current->counter = 0;
    _schedule();
}