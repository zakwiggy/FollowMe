#include "timer0.h"
#include "button.h"
#include "printf_P.h"


#ifdef USE_FOLLOWME
#define BUTTON       !(PINC & (1<<PINC6))
#endif
#ifdef USE_SDLOGGER
#define BUTTON       !(PINC & (1<<PINC3))
#endif

#define CNT_KEY     10 // at least 3
#define KEY_DELAY_MS  50

uint16_t ButtonTimer = 0;

void Button_Init(void)
{
	printf("\r\n BUTTON init...");
	// set port pin as input pullup
	#ifdef USE_FOLLOWME
	PORTC |= (1 << PORTC6);
	DDRC &= ~(1 << DDC6);
	#endif

	#ifdef USE_SDLOGGER
	PORTC |= (1 << PORTC3);
	DDRC &= ~(1 << DDC3);
	#endif
 	ButtonTimer = SetDelay(KEY_DELAY_MS);
 	printf("ok");
}

uint8_t GetButton(void)
{
	static uint8_t button = 0;
	uint8_t ret = 0;

	if(CheckDelay(ButtonTimer))
	{
		if(BUTTON)
		{
			if(button++ == 0 || button == CNT_KEY) ret = 1;
			if(button == CNT_KEY) button = CNT_KEY - CNT_KEY / 3;
		}
		else button = 0;
		ButtonTimer = SetDelay(KEY_DELAY_MS);
	}
	return(ret);
}

