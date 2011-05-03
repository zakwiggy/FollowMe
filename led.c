#include <inttypes.h>
#include "led.h"


// initializes the LED control outputs
void LED_Init(void)
{
	#ifdef USE_SDLOGGER
    // set PB0 as output (control of red LED)
	DDRB |= (1<<DDB0);
	LEDRED_OFF;
	#endif

	#ifdef USE_FOLLOWME
	// set PB0 as output (control of green LED)
	DDRB |= (1<<DDB0);
	LEDGRN_OFF;
	// set PB1 as output (control of red LED)
	DDRB |= (1<<DDB1);
	LEDRED_OFF;
	#endif

}
