#ifndef _MENU_H
#define _MENU_H

#include <inttypes.h>

#define DISPLAYBUFFSIZE 80

extern void Menu_Update(uint8_t Keys);
extern void Menu_Clear(void);
extern int8_t DisplayBuff[DISPLAYBUFFSIZE];
extern uint8_t DispPtr;
extern uint8_t MenuItem;
extern uint8_t MaxMenuItem;
#endif //_MENU_H


