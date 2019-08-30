#include "printf.h"
#include "string.h"
#include "uart.h"
#include "uart_boot.h"
#include "utils.h"

#define BUFF_SIZE 100
#define CHAIN_LOADING_ADDRESS ((char *)0x8000)

void kernel_main(void) {
    int cpuid = get_cpuid();

    uart_init();
    init_printf(0, putc);

    char buffer[BUFF_SIZE];
    readline(buffer, BUFF_SIZE);

    if (strcmp(buffer, "kernel") == 0) {
        copy_current_kernel_and_jump(CHAIN_LOADING_ADDRESS);
    }

    printf("Hello from CPU %d\r\n", cpuid);

    while (1) {
        uart_send(uart_recv());
    }
}