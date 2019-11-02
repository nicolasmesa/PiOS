#include "fork.h"
#include "irq.h"
#include "printf.h"
#include "sched.h"
#include "string.h"
#include "sys.h"
#include "timer.h"
#include "uart.h"
#include "uart_boot.h"
#include "user.h"
#include "utils.h"

#define BUFF_SIZE 100
#define CHAIN_LOADING_ADDRESS ((char *)0x8000)

// When this function finishes, it returns to the ret_from_fork function and
// executes the ret_to_user function.
void kernel_process() {
    printf("Kernel process started. EL %d\r\n", get_el());

    unsigned long begin = (unsigned long)&user_begin;
    unsigned long end = (unsigned long)&user_end;
    unsigned long process = (unsigned long)&user_process;

    printf("Calling move_to_user_mode(%x, %x, %x)\r\n", begin, end - begin,
           process - begin);

    // Here, we compute an offset of where the user_process function relative to
    // the user_begin portion.
    int err = move_to_user_mode(begin, end - begin, process - begin);
    if (err < 0) {
        printf("Error while moving process to user mode\r\n");
    }
}

void kernel_main(void) {
    uart_init();
    init_printf(0, putc);

    char buffer[BUFF_SIZE];
    readline(buffer, BUFF_SIZE);

    if (strcmp(buffer, "kernel") == 0) {
        copy_current_kernel_and_jump(CHAIN_LOADING_ADDRESS);
    }

    int cpuid = get_cpuid();
    int el = get_el();
    printf("Hello from CPU %d\r\n", cpuid);
    printf("Exception level: %d\r\n", el);

    irq_vector_init();
    timer_init();
    enable_interrupt_controller();
    enable_irq();

    int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0);
    if (res < 0) {
        printf("error while starting kernel process\r\n");
        return;
    }

    while (1) {
        // Once we call schedule for the first time, since current points to the
        // init task, we become the init task. So, everytime init runs, it's
        // actually running this while loop. Here, we're voluntarily giving up
        // the cpu.
        schedule();
    }
}