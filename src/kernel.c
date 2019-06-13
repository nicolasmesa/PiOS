 #include "mini_uart.h"

 #define BAUD_RATE (115200)

 void kernel_main(void) {
     uart_init(BAUD_RATE);
     uart_send_string("Hello world!\r\n");

     for(int i = 0; i < 1000; i++) {
       uart_send_string("Hello world!\r\n");
     }

     while (1) {
         uart_send(uart_recv());
     }
 }