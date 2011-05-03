#include <avr/io.h>
#include <avr/interrupt.h>

#include "main.h"
#include "uart1.h"
#include "printf_P.h"
#include "ubx.h"

fifo_t rxFifo;
unsigned char rxBuffer1[1000];
/****************************************************************/
/*              Initialization of the USART1                    */
/****************************************************************/
void USART1_Init (void)
{
	printf("\r\n UART1 init...");
	fifo_init (&rxFifo, rxBuffer1,999);
	// USART1 Control and Status Register A, B, C and baud rate register
	uint8_t sreg = SREG;
	uint16_t ubrr = (uint16_t) ((uint32_t) SYSCLK/(8 * USART1_BAUD) - 1);

	// disable all interrupts before reconfiguration
	cli();

	// disable RX-Interrupt
	UCSR1B &= ~(1 << RXCIE1);
	// disable TX-Interrupt
	UCSR1B &= ~(1 << TXCIE1);
	// disable DRE-Interrupt
	UCSR1B &= ~(1 << UDRIE1);

	// set direction of RXD1 and TXD1 pins
	// set RXD1 (PD2) as an input pin
	PORTD |= (1 << PORTD2);
	DDRD &= ~(1 << DDD2);

	// set TXD1 (PD3) as an output pin
	PORTD |= (1 << PORTD3);
	DDRD  |= (1 << DDD3);

	// USART0 Baud Rate Register
	// set clock divider
	UBRR1H = (uint8_t)(ubrr>>8);
	UBRR1L = (uint8_t)ubrr;

	// enable double speed operation
	UCSR1A |= (1 << U2X1);
	// enable receiver and transmitter
	UCSR1B = (1 << TXEN1) | (1 << RXEN1);
	// set asynchronous mode
	UCSR1C &= ~(1 << UMSEL11);
	UCSR1C &= ~(1 << UMSEL10);
	// no parity
	UCSR1C &= ~(1 << UPM11);
	UCSR1C &= ~(1 << UPM10);
	// 1 stop bit
	UCSR1C &= ~(1 << USBS1);
	// 8-bit
	UCSR1B &= ~(1 << UCSZ12);
	UCSR1C |=  (1 << UCSZ11);
	UCSR1C |=  (1 << UCSZ10);

	// flush receive buffer explicit
	while ( UCSR1A & (1<<RXC1) ) UDR1;

	// enable interrupts at the end
	// enable RX-Interrupt
	UCSR1B |= (1 << RXCIE1);
	// enable TX-Interrupt
	UCSR1B |= (1 << TXCIE1);
	// enable DRE interrupt
	//UCSR1B |= (1 << UDRIE1);


	// restore global interrupt flags
    SREG = sreg;
	sei();
	printf("ok");
}

/****************************************************************/
/*               USART1 transmitter ISR                         */
/****************************************************************/
/*ISR(USART1_TX_vect)
{

}
*/
/****************************************************************/
/*               USART1 receiver ISR                            */
/****************************************************************/
ISR(USART1_RX_vect)
{fifo_put(&rxFifo,UDR1);
	
	 // and put it into the ubx protocol parser

}
