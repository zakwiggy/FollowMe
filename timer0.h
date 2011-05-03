#ifndef _TIMER0_H
#define _TIMER0_H

#include <inttypes.h>

typedef struct{
	uint16_t	Year;
	uint8_t		Month;
	uint8_t		Day;
	uint8_t		Hour;
	uint8_t		Min;
	uint8_t		Sec;
	uint16_t	mSec;
	uint8_t		Valid;
}  DateTime_t;

extern DateTime_t SystemTime;

extern volatile uint16_t CountMilliseconds;

extern volatile uint16_t BeepTime;
extern volatile uint16_t BeepModulation;

extern void TIMER0_Init(void);
extern void Delay_ms(uint16_t w);
extern void Delay_ms_Mess(uint16_t w);
extern uint16_t SetDelay (uint16_t t);
extern int8_t CheckDelay (uint16_t t);

#endif //_TIMER0_H
