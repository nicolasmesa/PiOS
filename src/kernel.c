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

 #define BUFF_SIZE 100

 void kernel_main(void) {
     uart_init();

     char buffer[BUFF_SIZE];

     readline(buffer, BUFF_SIZE);
     if (strcmp(buffer, "kernel") == 0) {
         copy_and_jump_to_kernel();
     }

     uart_send_string("Hello world!\r\n");

     while (1) {
         uart_send(uart_recv());
     }
 }