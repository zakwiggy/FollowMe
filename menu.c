// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// + Copyright (c) 04.2007 Holger Buss
// + only for non-profit use
// + www.MikroKopter.com
// + see the File "License.txt" for further Informations
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <stdlib.h>
#include <inttypes.h>
#include "uart0.h"
#include "printf_P.h"
#include "ubx.h"
#include "timer0.h"
#include "main.h"

uint8_t MaxMenuItem = 3;
uint8_t MenuItem = 0;

#define KEY1    0x01
#define KEY2    0x02
#define KEY3    0x04
#define KEY4    0x08
#define KEY5    0x10



#define DISPLAYBUFFSIZE 80
int8_t DisplayBuff[DISPLAYBUFFSIZE] = "Hello World";
uint8_t DispPtr = 0;


/************************************/
/*        Clear LCD Buffer          */
/************************************/
void Menu_Clear(void)
{
	uint8_t i;
	for( i = 0; i < DISPLAYBUFFSIZE; i++) DisplayBuff[i] = ' ';
}


/************************************/
/*        Update Menu on LCD        */
/************************************/
// Display with 20 characters in 4 lines
void Menu_Update(uint8_t Keys)
{
	int16_t i1,i2,i3;
	uint8_t sign;

	if(Keys & KEY1)
	{
		if(MenuItem) MenuItem--;
		else MenuItem = MaxMenuItem;
	}
	if(Keys  & KEY2)
	{
		if(MenuItem == MaxMenuItem) MenuItem = 0;
		else MenuItem++;
	}
	/*
	if(Keys  & KEY4)
	{
		switch(SysState)
		{
			case STATE_IDLE:
				SysState = STATE_SEND_FOLLOWME; // activate followme only of no error has occured
				break;

			case STATE_SEND_FOLLOWME:
				SysState = STATE_IDLE;
				break;

			default:
				SysState = STATE_IDLE;
				break;
		}
	}*/
	if((Keys  & KEY1) && (Keys  & KEY2)) MenuItem = 0;

	Menu_Clear();

	if(MenuItem > MaxMenuItem) MenuItem = MaxMenuItem;
	// print menu item number in the upper right corner
	if(MenuItem < 10)
	{
	  LCD_printfxy(17,0,"[%i]",MenuItem);
	}
	else
	{
	  LCD_printfxy(16,0,"[%i]",MenuItem);
	}

	switch(MenuItem)
	{
    case 0:// Version Info Menu Item
           LCD_printfxy(0,0,"+ Follow Me +");
           #ifdef USE_SDLOGGER
           LCD_printfxy(0,1,"HW: SD-Logger");
           #endif
           #ifdef USE_FOLLOWME
           LCD_printfxy(0,1,"HW: Follow-Me");
           #endif
           LCD_printfxy(0,2,"SW: %d.%d%c", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH+'a');
           LCD_printfxy(0,3,"          ");
           break;
	case 1:
		if (GPSData.Status == INVALID)
		{
			LCD_printfxy(0,0,"No GPS data");
			LCD_printfxy(0,1,"Lon:                ");
			LCD_printfxy(0,2,"Lat:                ");
			LCD_printfxy(0,3,"Alt:                ");
		}
		else // newdata or processed
		{
			switch (GPSData.SatFix)
			{
			case SATFIX_NONE:
				LCD_printfxy(0,0,"Sats:%02d Fix:None", GPSData.NumOfSats);
				break;
			case SATFIX_2D:
				LCD_printfxy(0,0,"Sats:%02d Fix:2D  ", GPSData.NumOfSats);
				break;
			case SATFIX_3D:
				LCD_printfxy(0,0,"Sats:%02d Fix:3D  ", GPSData.NumOfSats);
				break;
			default:
				LCD_printfxy(0,0,"Sats:%02d Fix:??  ", GPSData.NumOfSats);
				break;
			}
			if(GPSData.Position.Longitude < 0) sign = '-';
			else sign = '+';
			i1 = abs((int16_t)(GPSData.Position.Longitude/10000000L));
			i2 = abs((int16_t)((GPSData.Position.Longitude%10000000L)/10000L));
			i3 = abs((int16_t)(((GPSData.Position.Longitude%10000000L)%10000L)/10L));
			LCD_printfxy(0,1,"Lon: %c%d.%.3d%.3d deg",sign, i1, i2, i3);
			if(GPSData.Position.Latitude < 0) sign = '-';
			else sign = '+';
			i1 = abs((int16_t)(GPSData.Position.Latitude/10000000L));
			i2 = abs((int16_t)((GPSData.Position.Latitude%10000000L)/10000L));
			i3 = abs((int16_t)(((GPSData.Position.Latitude%10000000L)%10000L)/10L));
			LCD_printfxy(0,2,"Lat: %c%d.%.3d%.3d deg",sign, i1, i2, i3);
			if(GPSData.Position.Altitude < 0) sign = '-';
			else sign = '+';
			i1 = abs((int16_t)(GPSData.Position.Altitude/1000L));
			i2 = abs((int16_t)(GPSData.Position.Altitude%1000L));
			LCD_printfxy(0,3,"Alt: %c%04d.%.03d m",sign, i1, i2);
		}
		break;
	case 2:
		if (GPSData.Status == INVALID)
		{
			LCD_printfxy(0,0,"No GPS data");
			LCD_printfxy(0,1,"Speed N:            ");
			LCD_printfxy(0,2,"Speed E:            ");
			LCD_printfxy(0,3,"Speed T:            ");
		}
		else // newdata or processed
		{
			switch (GPSData.SatFix)
			{
			case SATFIX_NONE:
				LCD_printfxy(0,0,"Sats:%02d Fix:None", GPSData.NumOfSats);
				break;
			case SATFIX_2D:
				LCD_printfxy(0,0,"Sats:%02d Fix:2D  ", GPSData.NumOfSats);
				break;
			case SATFIX_3D:
				LCD_printfxy(0,0,"Sats:%02d Fix:3D  ", GPSData.NumOfSats);
				break;
			default:
				LCD_printfxy(0,0,"Sats:%02d Fix:??  ", GPSData.NumOfSats);
				break;
			}
			LCD_printfxy(0,1,"Speed N: %+4d cm/s",(int16_t)GPSData.Speed_North);
			LCD_printfxy(0,2,"Speed E: %+4d cm/s",(int16_t)GPSData.Speed_East);
			LCD_printfxy(0,3,"Speed T: %+4d cm/s",(int16_t)GPSData.Speed_Top);
		}
		break;
	case 3:
		LCD_printfxy(0,0,"GPS UTC Time");
		if (!SystemTime.Valid)
		{
			LCD_printfxy(0,1,"                    ");
			LCD_printfxy(0,2,"  No time data!     ");
			LCD_printfxy(0,3,"                    ");
		}
		else // newdata or processed
		{
			LCD_printfxy(0,1,"                    ");
			LCD_printfxy(0,2,"Date: %02i/%02i/%04i",SystemTime.Month, SystemTime.Day, SystemTime.Year);
			LCD_printfxy(0,3,"Time: %02i:%02i:%02i.%03i", SystemTime.Hour, SystemTime.Min, SystemTime.Sec, SystemTime.mSec);
		}
	break;

    default: MaxMenuItem = MenuItem - 1;
             MenuItem = 0;
           break;
    }
}
