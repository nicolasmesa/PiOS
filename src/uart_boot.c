#include "uart.h"
#include "utils.h"

// See https://sourceware.org/binutils/docs/ld/Source-Code-Reference.html
extern char bss_end[];

void copy_and_jump_to_kernel() {
    int kernel_size = uart_read_int();

    // Confirm kernel size
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

/**
 * This is a weird function.  It copies everything from 0x00
 * up to bss_end to the new_address. Then, it gets the address of
 * copy_and_jump_to_kernel and adds the offset of the new address. We do this
 * because we want to call the function in the new address (the newly copied
 * kernel).
 */
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