#ifndef _BUTTON_H
#define _BUTTON_H

#include <avr/io.h>
#include <inttypes.h>

extern void Button_Init(void);
extern uint8_t GetButton(void);


#endif //_BUTTON_H
