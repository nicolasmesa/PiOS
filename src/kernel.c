#include "uart.h"
#include "utils.h"

#define BUFF_SIZE 100
// The magic address is an arbitrary memory address that we use
// to let other processors know what to do. There are four states (or values):
// 0. We initialize this to 0 while we decide if we're going to copy a new
//  kernel or if we cant to keep going in the current kernel.
// 1. We just finished copying the current kernel over to the new address and
//  other CPUs should jump to start executing code in that newly copied kernel
//  while CPU 1 copies the new kernel (overwriting the old one).
// 2. We're done copying the new kernel so the CPUs should jump back to address
//  0 to start executing the new kernel. This address can have values 0, 1, 2.
// 3. We're not copying a new kernel, so continue with the execution of the
// current kernel.
// TLDR
// 0 -- Loading
// 1 -- Copied current kernel to new place (jump there).
// 2 -- Copied new kernel to new place. (jump to 0).
// 3 -- Not going to copy anything, keep going.
#define MAGIC_ADDRESS ((char *)0xD000)

void hang() {
    while (1)
        ;
}

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
        // It seems like screen sends \r when I press enter
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

    // Set this to one (see scenario 1 above) to make other CPUs jump to this
    // address too.
    *MAGIC_ADDRESS = 2;
    branch_to_address((void *)0x00);
}

/**
 * This is a weird function. It assumes that the currently executing kernel is
 * at address 0x00 and is not bigger than 16KB. It copies everything from 0x00
 * up to 0x3fff to the new_address. Then, it gets the address of
 * copy_and_jump_to_kernel and adds the offset of the new address. We do this
 * because we want to call the function in the new address (the newly copied
 * kernel).
 */
void copy_current_kernel_and_jump(char *new_address) {
    // TODO: We need actual values here instead of blindly copying 16KB
    char *kernel = (char *)0x00;
    char *end =
        (char *)0x4000;  // Copy 16KB (a lot but should be enough for now)

    char *copy = new_address;

    while (kernel < end) {
        *copy = *kernel;
        kernel++;
        copy++;
    }

    // Cast the function pointer to char* to deal with bytes.
    char *original_function_address = (char *)&copy_and_jump_to_kernel;

    // Allow the other CPUs to jump to the copied section
    *MAGIC_ADDRESS = 1;
    delay(10000);

    // Add the new address (we're assuming that the original kernel resides in
    // address 0). copied_function_address should now contain the address of the
    // original function but in the new location.
    char *copied_function_address =
        original_function_address + (long)new_address;

    // Cast the address back to a function and call it.
    void (*call_function)() = (void (*)())copied_function_address;
    call_function();
}

void handle_cpu_and_jump() {
    while (*MAGIC_ADDRESS == 1)
        ;

    branch_to_address((void *)0x00);  // never returns;
}

int init_step = 0;

// 0 -- Loading
// 1 -- Copied current kernel to new place (jump there)
// 2 -- Copied new kernel to new place. (jump to 0)
// 3 -- Not goint to copy anything, keep going.
void handle_cpu(int cpuid, char *new_address) {
    if (cpuid == 0) {
        return;
    }

    // Required to avoid race conditions with MAGIC_ADDRESS. We only continue
    // when we're certain that the value of MAGIC_ADDRESS has been set to 0.
    while (init_step == 0)
        ;

    while (*MAGIC_ADDRESS == 0)
        ;

    // We're not going to copy the kernel so we'll return
    if (*MAGIC_ADDRESS == 3) {
        return;
    }

    char *original_function_address = (char *)handle_cpu_and_jump;

    // Add the new address (we're assuming that the original kernel resides in
    // address 0). copied_function_address should now contain the address of the
    // original function but in the new location.
    char *copied_function_address =
        original_function_address + (long)new_address;

    // Cast the address back to a function and call it.
    void (*call_function)() = (void (*)())copied_function_address;
    call_function();

    branch_to_address((void *)0x00);  // never returns;
}

void kernel_main(void) {
    char *new_address = (char *)0x8000;
    int cpuid = get_cpuid();

    if (cpuid == 0) {
        uart_init();
        *MAGIC_ADDRESS = 0;
        init_step = 1;
    }

    handle_cpu(cpuid, new_address);

    char buffer[BUFF_SIZE];
    if (cpuid == 0) {
        readline(buffer, BUFF_SIZE);

        if (strcmp(buffer, "kernel") == 0) {
            copy_current_kernel_and_jump(new_address);
        }
        *MAGIC_ADDRESS = 3;
    }

    // Each CPU will increment init_step when it's done sending the string
    // We use this to make sure only one CPU is sending at a time.
    while (init_step < (cpuid + 1))
        ;

    uart_send_string("Hello from CPU ");
    uart_send(cpuid + '0');
    uart_send_string("\r\n");
    init_step++;

    if (cpuid == 0) {
        while (1) {
            uart_send(uart_recv());
        }
    } else {
        // hang other CPUs
        while (1)
            ;
    }
}