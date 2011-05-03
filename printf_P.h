#ifndef _PRINTF_P_H_
#define _PRINTF_P_H_

#include <avr/pgmspace.h>

#define OUT_V24   0
#define OUT_LCD   1


void _printf_P (char, char const *fmt0, ...);
extern char PrintZiel;


#define printf_P(format, args...)   _printf_P(OUT_V24,format , ## args)
#define printf(format, args...)     _printf_P(OUT_V24,PSTR(format) , ## args)
#define LCD_printfxy(x,y,format, args...)  { DispPtr = y * 20 + x; _printf_P(OUT_LCD,PSTR(format) , ## args);}
#define LCD_printf(format, args...)  {  _printf_P(OUT_LCD,PSTR(format) , ## args);}

#endif
