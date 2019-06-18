 #include "mini_uart.h"

 #define BAUD_RATE (115200)

 void kernel_main(void) {
     mini_uart_init(BAUD_RATE);
     mini_uart_send_string("Hello world!\r\n");

     for(int i = 0; i < 1000; i++) {
       mini_uart_send_string("Hello world!\r\n");
     }

     while (1) {
         mini_uart_send(mini_uart_recv());
     }
 }