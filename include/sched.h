#ifndef _SCHED_H
#define _SCHED_H

#define THREAD_CPU_CONTEXT 0  // offset of cpu_context in task_struct

#ifndef __ASSEMBLER__

#define THREAD_SIZE 4096

// Max number of tasks.
#define NR_TASKS 64

#define FIRST_TASK (task[0])
#define LAST_TASK (task[NR_TASKS - 1])

#define TASK_RUNNING 0
#define TASK_ZOMBIE 1

#define PF_KTHREAD 0x00000002

extern struct task_struct *current;
extern struct task_struct *task[NR_TASKS];
extern int nr_tasks;

// We don't save registers x0 - x18 because we switch CPU context via a function
// call. ARM conventions say that, when calling a function, registers x0 - x18
// may be overwritten. As a result, it is up to the caller to decide which of
// these registers to preserve.
struct cpu_context {
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp;  // or x29
    unsigned long sp;
    unsigned long pc;  // or x30
};

struct user_page {
    unsigned long phys_addr;
    unsigned long virt_addr;
};

#define MAX_PROCESS_PAGES 16

struct mm_struct {
    // Pointer to the pgd of this task (Physical address).
    unsigned long pgd;

    // We kee track of the user pages to make it easy to copy them when the task
    // calls fork.
    int user_pages_count;
    struct user_page user_pages[MAX_PROCESS_PAGES];

    // We need these to keep track of the pages that the task is using to be
    // able to free them whe we're done. These pages are for PGD/PUD/...
    int kernel_pages_count;
    unsigned long kernel_pages[MAX_PROCESS_PAGES];
};

struct task_struct {
    struct cpu_context cpu_context;

    // Holds the task state (TASK_RUNNING/etc).
    long state;

    // holds how many ticks are left for the task to run. When it reaches 0, its
    // time slice is up and another task is scheduled.
    long counter;

    // This is the time slice to give the task every time it's scheduled (after
    // the time slice reaches 0). This helps determine the priority of the task
    // because the more priority, the larger its time slice.
    long priority;

    // Indicates if the current task is preemptable or not. It may not be
    // preemptable if the task is currently running in the scheduler, for
    // example. Note that it is a counter because preempt_disable could be
    // called multiple times.
    long preempt_count;

    // Custom field added by me
    int pid;

    unsigned long flags;

    struct mm_struct mm;
};

extern void preempt_disable(void);
extern void preempt_enable(void);
extern void schedule_tail(void);
extern void timer_tick();
extern void cpu_switch_to(struct task_struct *, struct task_struct *);
extern void schedule(void);
extern void exit_process();

#define INIT_TASK                                                  \
    {                                                              \
        /*cpu_context*/ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   \
            /* state etc */ 0, 0, 15, 0, 0, PF_KTHREAD, /* mm */ { \
            0, 0, {{0}}, 0, { 0 }                                  \
        }                                                          \
    }

#endif
#endif