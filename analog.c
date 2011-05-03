
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "analog.h"
#include "printf_P.h"

volatile uint16_t Adc0, Adc1, Adc2, Adc3, Adc4, Adc5, Adc6, Adc7;
volatile uint8_t ADReady = 1;

/*****************************************************/
/*     Initialize Analog Digital Converter           */
/*****************************************************/
void ADC_Init(void)
{
	uint8_t sreg = SREG;
	printf("\r\n ADC init...");
	// disable all interrupts before reconfiguration
	cli();
	//ADC0 ... ADC7 is connected to PortA pin 0 ... 7
	DDRA = 0x00;
	PORTA = 0x00;
	// Digital Input Disable Register 0
	// Disable digital input buffer for analog adc_channel pins
	DIDR0 = 0xFF;
	// external reference AREF, adjust data to the right
    ADMUX &= ~((1 << REFS1)|(1 << REFS0)|(1 << ADLAR));
    // set muxer to ADC adc_channel 0 (0 to 7 is a valid choice)
    ADMUX = (ADMUX & 0xE0) | 0x00;
    //Set ADC Control and Status Register A
    //Auto Trigger Enable, Prescaler Select Bits to Division Factor 128, i.e. ADC clock = SYSCKL/128 = 156.25 kHz
  	ADCSRA = (0<<ADEN)|(0<<ADSC)|(0<<ADATE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(0<<ADIE);
	//Set ADC Control and Status Register B
	//Trigger Source to Free Running Mode
	ADCSRB &= ~((1 << ADTS2)|(1 << ADTS1)|(1 << ADTS0));
	// Start AD conversion
	ADC_Enable();
    // restore global interrupt flags
    SREG = sreg;
	sei();
    printf("ok");
}

/*****************************************************/
/*     Interrupt Service Routine for ADC             */
/*****************************************************/
// runs at 312.5 kHz or 3.2 µs
// if after (60.8µs) all 19 states are processed the interrupt is disabled
// and the update of further ads is stopped


#define ADC0	0
#define ADC1	1
#define ADC2	2
#define ADC3	3
#define ADC4	4
#define ADC5	5
#define ADC6	6
#define ADC7	7

ISR(ADC_vect)
{
    static uint8_t ad_channel = ADC0, state = 0;

    // state machine
	switch(state++)
	{
		case 0:
			Adc0 = ADC;
			ad_channel = ADC1;
			break;
		case 1:
			Adc1 = ADC;
			ad_channel = ADC2;
			break;
		case 2:
			Adc2 = ADC;
			ad_channel = ADC3;
			break;
		case 3:
			Adc3 = ADC;
			ad_channel = ADC4;
            break;
		case 4:
			Adc4 = ADC;
			ad_channel = ADC5;
			break;
		case 5:
			Adc5 = ADC;
			ad_channel = ADC6;
			break;
		case 6:
			Adc6 = ADC;
			ad_channel = ADC7;
			break;
		case 7:
			Adc7 = ADC;
			ad_channel = ADC0;
			state = 0;
			ADReady = 1;
            break;
		default:
			ad_channel = ADC0;
			state = 0;
			ADReady = 1;
			break;
	}
    // set adc muxer to next ad_channel
    ADMUX = (ADMUX & 0xE0) | ad_channel;
    // after full cycle stop further interrupts
    if(state != 0) ADC_Enable();
}
