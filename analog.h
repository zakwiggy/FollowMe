#ifndef _ANALOG_H
#define _ANALOG_H

#include <inttypes.h>

extern volatile uint16_t Adc0, Adc1, Adc2, Adc3, Adc4, Adc5, Adc6, Adc7;
extern volatile uint8_t ADReady;

void ADC_Init(void);


// clear ADC enable & ADC Start Conversion & ADC Interrupt Enable bit
#define ADC_Disable() (ADCSRA &= ~((1<<ADEN)|(1<<ADSC)|(1<<ADIE)))
// set ADC enable & ADC Start Conversion & ADC Interrupt Enable bit
#define ADC_Enable() (ADCSRA |= (1<<ADEN)|(1<<ADSC)|(1<<ADIE))


#endif //_ANALOG_H


