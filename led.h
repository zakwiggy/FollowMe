#ifndef _LED_H
#define _LED_H

#include <avr/io.h>

#ifdef USE_SDLOGGER
#define LEDRED_OFF   	PORTB |=  (1<<PORTB0)
#define LEDRED_ON		PORTB &= ~(1<<PORTB0)
#define LEDRED_TOGGLE	PORTB ^=  (1<<PORTB0)

#define LEDGRN_OFF
#define LEDGRN_ON
#define LEDGRN_TOGGLE
#endif

#ifdef USE_FOLLOWME
#define LEDGRN_OFF   	PORTB |=  (1<<PORTB1)
#define LEDGRN_ON		PORTB &= ~(1<<PORTB1)
#define LEDGRN_TOGGLE	PORTB ^=  (1<<PORTB1)

#define LEDRED_OFF   	PORTB |=  (1<<PORTB0)
#define LEDRED_ON		PORTB &= ~(1<<PORTB0)
#define LEDRED_TOGGLE	PORTB ^=  (1<<PORTB0)
#endif

void LED_Init(void);

#endif //_LED_H

