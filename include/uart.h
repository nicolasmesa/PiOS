#ifndef _UART_H
#define _UART_H

void uart_init();
void uart_send_string(char *str);
void uart_send(char c);
char uart_recv();
void uart_send_int(int number);
int uart_read_int();
void send_long_as_hex_string(long number);
void putc(void *p, char c);

#endif /*_UART_H */