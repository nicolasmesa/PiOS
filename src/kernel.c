 #include "uart.h"

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
        // Temporary
        uart_send(c);
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

 int read_int() {
     int num = 0;
     for (int i = 0; i < 4; i++) {
         char c = uart_recv();
         num = num << 8;
         num += (int)c;
     }
     return num;
 }

 #define BUFF_SIZE 100

 void kernel_main(void) {
     uart_init();

     char buffer[BUFF_SIZE];

     // Wait for user input before sending the Hello world

     readline(buffer, BUFF_SIZE);
     while (strcmp(buffer, "kernel") != 0) {
        uart_send_string("Not the same got: '");
        uart_send_string(buffer);
        uart_send_string("\r\n");
        readline(buffer, BUFF_SIZE);
     }

     uart_send_string("Hello world!\r\n");

     int x = read_int();
     char *p = (char *)&x;
     for (int i = 0; i < 4; i++) {
         uart_send(p[i]);
     }
     uart_send_string("\r\n");

     if (x == 1315529583) {
         uart_send_string("They are equal!");
     } else {
         uart_send_string("Not equal");
     }

     while (1) {
         uart_send(uart_recv());
     }
 }