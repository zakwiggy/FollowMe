#ifndef _MAIN_H
#define _MAIN_H

#include <avr/io.h>
#define SYSCLK F_CPU


typedef enum
{
	STATE_UNDEFINED,
	STATE_IDLE,
	STATE_SEND_FOLLOWME
} SysState_t;

#define ERROR_GPS_RX_TIMEOUT	0x0001
#define ERROR_LOW_BAT			0x0002

extern uint16_t Error;
extern int16_t UBat;
extern SysState_t SysState;

#endif //_MAIN_H







