#include "fork.h"
#include "irq.h"
#include "printf.h"
#include "sched.h"
#include "string.h"
#include "timer.h"
#include "uart.h"
#include "uart_boot.h"
#include "utils.h"

#define BUFF_SIZE 100
#define CHAIN_LOADING_ADDRESS ((char *)0x8000)

void process(char *str) {
    while (1) {
        int i = 0;
        char c;
        while ((c = str[i]) != '\0') {
            uart_send(c);
            i++;
            delay(100000);
        }
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

    int res = copy_process((unsigned long)&process, (unsigned long)"12345");
    if (res) {
        printf("error starting process 1");
        return;
    }

    res = copy_process((unsigned long)&process, (unsigned long)"abcde");
    if (res) {
        printf("error starting process 2");
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