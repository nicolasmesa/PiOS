 #include "uart.h"

 void kernel_main(void) {
     uart_init();
     uart_send_string("Hello world!\r\n");

     for(int i = 0; i < 1000; i++) {
       uart_send_string("Hello world!\r\n");
     }

     while (1) {
         uart_send(uart_recv());
     }
 }