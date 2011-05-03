#ifndef _UART1_H
#define _UART1_H

#define USART1_BAUD 57600

/*
Initialize the USART und activate the receiver and transmitter
as well as the receive-interrupt. The IO-FIFOs are initialized.
The global interrupt-enable-flag (I-Bit in SREG) is not changed
*/
extern void USART1_Init (void);

/*
The character c is stored in the output buffer. If the character was pushed sucessfully to
the output buffer then the return value is 1. In case of an output buffer overflow the return value is 0.
The isr is activated, which will send the data from the outbut buffer to the UART.
*/
extern int USART1_putc (const uint8_t c);

/*
extern uint8_t USART1_getc_wait(void);
extern int16_t USART1_getc_nowait(void);
*/

#endif //_UART1_H
