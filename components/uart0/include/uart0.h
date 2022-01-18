#ifndef __UART0_H_
#define __UART0_H_


//UART0
#define RX0_BUF_SIZE       (1024)
#define TX0_BUF_SIZE       (512)
#define RXD0_PIN           (3)
#define TXD0_PIN           (1)

void uart_init(void);

#endif