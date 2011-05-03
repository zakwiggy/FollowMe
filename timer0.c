#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "timer0.h"

volatile uint16_t CountMilliseconds = 0;
DateTime_t SystemTime;

volatile uint16_t BeepTime = 0;
volatile uint16_t BeepModulation = 0xFFFF;

/*****************************************************/
/*              Initialize Timer 0                   */
/*****************************************************/
// timer 0 is used for the PWM generation to control the offset voltage at the air pressure sensor
// Its overflow interrupt routine is used to generate the beep signal and the flight control motor update rate
void TIMER0_Init(void)
{
	uint8_t sreg = SREG;

	// disable all interrupts before reconfiguration
	cli();


	// configure speaker port as output

	#ifdef USE_FOLLOWME
	// Speaker at PC7
	DDRC |= (1<<DDC7);
	PORTC &= ~(1<<PORTC7);
	#endif

	// Timer/Counter 0 Control Register A

	// Waveform Generation None (Bits WGM02 = 0, WGM01 = 0, WGM00 = 0)
    TCCR0A &= ~((1<<COM0A0)|(1<<COM0B0)|(1<<COM0A1)|(1<<COM0B1)|(1<<WGM01)|(1<<WGM00));

	// Timer/Counter 0 Control Register B

	// set clock devider for timer 0 to SYSKLOCK/8 = 20MHz / 8 = 2.5MHz
	// i.e. the timer increments from 0x00 to 0xFF with an update rate of 2.5 MHz
	// hence the timer overflow interrupt frequency is 2.5 MHz / 256 = 9.765 kHz

	// divider 8 (Bits CS02 = 0, CS01 = 1, CS00 = 0)
	TCCR0B &= ~((1<<FOC0A)|(1<<FOC0B)|(1<<WGM02));
    TCCR0B = (TCCR0B & 0xF8)|(0<<CS02)|(1<<CS01)|(0<<CS00);

	// init Timer/Counter 0 Register
    TCNT0 = 0;

	// Timer/Counter 0 Interrupt Mask Register
	// enable timer overflow interrupt only
	TIMSK0 &= ~((1<<OCIE0B)|(1<<OCIE0A));
	TIMSK0 |= (1<<TOIE0);


	SystemTime.Year = 0;
	SystemTime.Month = 0;
	SystemTime.Day = 0;
	SystemTime.Hour = 0;
	SystemTime.Min = 0;
	SystemTime.Sec = 0;
	SystemTime.mSec = 0;
	SystemTime.Valid = 0;

	CountMilliseconds = 0;

	SREG = sreg;
	sei();
}



/*****************************************************/
/*          Interrupt Routine of Timer 0             */
/*****************************************************/
ISR(TIMER0_OVF_vect)    // 9.765 kHz
{
    static uint8_t cnt = 0;
    #ifdef USE_FOLLOWME
    uint8_t Beeper_On = 0;
	#endif

	if(!cnt--) // every 10th run (9.765kHz/10 = 976Hz)
	{
	 	cnt = 9;
	 	CountMilliseconds++; // increment millisecond counter
	}

	// beeper on if duration is not over
	if(BeepTime)
	{
		BeepTime--; // decrement BeepTime
		if(BeepTime & BeepModulation) Beeper_On = 1;
		else Beeper_On = 0;
	}
	else // beeper off if duration is over
	{
		Beeper_On = 0;
		BeepModulation = 0xFFFF;
	}
	 #ifdef USE_FOLLOWME
	// if beeper is on
	if(Beeper_On)
	{
		// set speaker port to high
		PORTC |= (1<<PORTC7); // Speaker at PC7
	}
	else // beeper is off
	{
		// set speaker port to low
		PORTC &= ~(1<<PORTC7);// Speaker at PC7
	}
	#endif
}


// -----------------------------------------------------------------------
uint16_t SetDelay (uint16_t t)
{
  return(CountMilliseconds + t - 1);
}

// -----------------------------------------------------------------------
int8_t CheckDelay(uint16_t t)
{
  return(((t - CountMilliseconds) & 0x8000) >> 8); // check sign bit
}

// -----------------------------------------------------------------------
void Delay_ms(uint16_t w)
{
 unsigned int t_stop;
 t_stop = SetDelay(w);
 while (!CheckDelay(t_stop));
}

