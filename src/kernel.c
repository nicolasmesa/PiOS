#include "uart.h"
#include "utils.h"

extern char bss_end[];

int strcmp(char *str1, char *str2) {
    while (1) {
        if (*str1 != *str2) {
            return *str1 - *str2;
        }

        if (*str1 == '\0') {
            return 0;
        }

        str1++;
        str2++;
    }
}

int readline(char *buf, int maxlen) {
    int num = 0;
    while (num < maxlen - 1) {
        char c = uart_recv();
        if (c == '\n' || c == '\0' || c == '\r') {
            break;
        }
        buf[num] = c;
        num++;
    }
    buf[num] = '\0';
    return num;
}

void copy_and_jump_to_kernel() {
    int kernel_size = uart_read_int();
    uart_send_int(kernel_size);

    char *kernel = (char *)0;
    int checksum = 0;
    for (int i = 0; i < kernel_size; i++) {
        char c = uart_recv();
        checksum += c;
        kernel[i] = c;
    }
    uart_send_int(checksum);

    uart_send_string("Done copying kernel\r\n");
    branch_to_address((void *)0x00);
}

void copy_current_kernel_and_jump(char *new_address) {
    char *kernel = (char *)0x00;
    char *end = bss_end;

    char *copy = new_address;

    while (kernel <= end) {
        *copy = *kernel;
        kernel++;
        copy++;
    }

    // Cast the function pointer to char* to deal with bytes.
    char *original_function_address = (char *)&copy_and_jump_to_kernel;

    // Add the new address (we're assuming that the original kernel resides in
    // address 0). copied_function_address should now contain the address of the
    // original function but in the new location.
    char *copied_function_address =
        original_function_address + (long)new_address;

    // Cast the address back to a function and call it.
    void (*call_function)() = (void (*)())copied_function_address;
    call_function();
}

void my_test_function(void) { uart_send_string("Sending a test message!\r\n"); }

void kernel_main(void) {
    int buff_size = 100;
    uart_init();

    char buffer[buff_size];
    readline(buffer, buff_size);
    if (strcmp(buffer, "kernel") == 0) {
        copy_current_kernel_and_jump((char *)0x8000);
    }

    uart_send_string("Hello from a new kernel!!!\r\n");
    my_test_function();

    while (1) {
        uart_send(uart_recv());
    }
}