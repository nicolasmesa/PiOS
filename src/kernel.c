 #include "uart.h"
 #include "utils.h"

void hang() {
    while (1);
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
     while (num < maxlen) {
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

    branch_to_address((void *)0x00);
}

/**
 * This is a weird function. It assumes that the currently executing kernel is at
 * address 0x00 and is not bigger than 16KB. It copies everything from 0x00 up to 0x3fff
 * to the new_address. Then, it gets the address of copy_and_jump_to_kernel and adds
 * the offset of the new address. We do this because we want to call the function in
 * the new address (the newly copied kernel).
 */
void copy_current_kernel_and_jump(char *new_address) {
    // TODO: We need actual values here instead of blindly copying 16KB
    char *kernel = (char *) 0x00;
    char *end = (char *) 0x4000; // Copy 16KB (a lot but should be enough for now)

    char *copy = new_address;

    while(kernel < end) {
        *copy = *kernel;
        kernel++;
        copy++;
    }

    // Cast the function pointer to char* to deal with bytes.
    char *original_function_address = (char *)&copy_and_jump_to_kernel;

    // Add the new address (we're assuming that the original kernel resides in address 0).
    // copied_function_address should now contain the address of the original function but
    // in the new location.
    char *copied_function_address = original_function_address + (long) new_address;

    // Cast the address back to a function and call it.
    void (*call_function)() =  (void (*)()) copied_function_address;
    call_function();
}

 #define BUFF_SIZE 100

 void kernel_main(void) {
     uart_init();

     char buffer[BUFF_SIZE];

     readline(buffer, BUFF_SIZE);
     if (strcmp(buffer, "kernel") == 0) {
         copy_current_kernel_and_jump((char *) 0x8000);
     }

     uart_send_string("Hello world!\r\n");

     while (1) {
         uart_send(uart_recv());
     }
 }