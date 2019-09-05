#include "fork.h"
#include "irq.h"
#include "printf.h"
#include "sched.h"
#include "string.h"
#include "sys.h"
#include "timer.h"
#include "uart.h"
#include "uart_boot.h"
#include "utils.h"

#define BUFF_SIZE 100
#define CHAIN_LOADING_ADDRESS ((char *)0x8000)

void user_process1(char *str) {
    char buf[2];
    while (1) {
        char c;
        int i = 0;
        while ((c = str[i++]) != '\0') {
            buf[0] = c;
            buf[1] = '\0';
            call_sys_write(buf);
            delay(100000);
        }
    }
}

void user_process2(char *str) {
    int i = 100;
    char buf[2];
    while (i--) {
        char c;
        int i = 0;
        while ((c = str[i++]) != '\0') {
            buf[0] = c;
            buf[1] = '\0';
            call_sys_write(buf);
            delay(100000);
        }
    }
    char buf2[30] = {0};
    tfp_sprintf(buf2, "Done with user process 2\n\r");
    call_sys_write(buf2);
}

void user_process() {
    char buf[30] = {0};
    tfp_sprintf(buf, "User process started\n\r");
    call_sys_write(buf);

    unsigned long stack = call_sys_malloc();
    if (stack < 0) {
        // printf shouldn't be allowed here
        tfp_sprintf(buf, "Error while allocating stack for process 1\r\n");
        call_sys_write(buf);
        return;
    }

    int err = call_sys_clone((unsigned long)&user_process1,
                             (unsigned long)"abcd", stack);
    if (err < 0) {
        tfp_sprintf(buf, "Error calling sys_clone 1\r\n");
        call_sys_write(buf);
        return;
    }

    stack = call_sys_malloc();
    if (stack < 0) {
        // printf shouldn't be allowed here
        tfp_sprintf(buf, "Error while allocating stack for process 2\r\n");
        call_sys_write(buf);
        return;
    }

    err = call_sys_clone((unsigned long)&user_process1, (unsigned long)"1234",
                         stack);

    if (err < 0) {
        tfp_sprintf(buf, "Error calling sys_clone 2\r\n");
        call_sys_write(buf);
        return;
    }

    stack = call_sys_malloc();
    if (stack < 0) {
        // printf shouldn't be allowed here
        tfp_sprintf(buf, "Error while allocating stack for process 3\r\n");
        call_sys_write(buf);
        return;
    }

    err = call_sys_clone((unsigned long)&user_process2, (unsigned long)"wxyz",
                         stack);

    if (err < 0) {
        tfp_sprintf(buf, "Error calling sys_clone 3\r\n");
        call_sys_write(buf);
        return;
    }

    // We need to call this explicitly here because this process doesn't go
    // though thread_start in sys.S. If we don't call this, we will crash since
    // this will return to address 0x00 since set to 0 (using memzero) in
    // the move_to_user_mode function.
    call_sys_exit();
}

// When this function finishes, it returns to the ret_from_fork function and
// executes the ret_to_user function.
void kernel_process() {
    printf("Kernel process started. EL %d\r\n", get_el());

    int err = move_to_user_mode((unsigned long)&user_process);
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

    int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0, 0);
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