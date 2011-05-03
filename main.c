#include <avr/boot.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include "ubx.h"
#include "main.h"
#include "timer0.h"
#include "uart0.h"
#include "uart1.h"
#include "fat16.h"
#include "led.h"
#include "menu.h"
#include "printf_P.h"
#include "analog.h"
#include "gps.h"
#include "button.h"
#include "logging.h"
#include "settings.h"

#define FOLLOWME_INTERVAL 1000 // 1 second update
#define CELLUNDERVOLTAGE 32 // lowest allowed voltage/cell; 32 = 3.2V

#ifdef USE_FOLLOWME
int16_t UBat = 120;
int16_t Zellenzahl = 0;
int16_t PowerOn = 0;
int16_t i = 0;
int16_t delay = 0;
#endif

uint16_t Error = 0;
SysState_t SysState = STATE_UNDEFINED;

int main (void)
{
	static uint16_t FollowMe_Timer = 0;

	// disable interrupts global
	//cli();

	// disable watchdog
    MCUSR &=~(1<<WDRF);
    WDTCSR |= (1<<WDCE)|(1<<WDE);
    WDTCSR = 0;

	// initalize modules
	LED_Init();
	LEDRED_ON;
    TIMER0_Init();
	USART0_Init();
	UBX_Init();
	USART1_Init();
	ADC_Init();
	Button_Init();
	// enable interrupts global
	sei();

	// try to initialize the FAT 16 filesystem on the SD-Card
	Fat16_Init();
	// initialize the settings
	Settings_Init();
	// initialize logging (needs settings)
	Logging_Init();

	LEDRED_OFF;
	LEDGRN_ON;

  	#ifdef USE_SDLOGGER
	printf("\r\n\r\nHW: SD-Logger");
	#endif
	#ifdef USE_FOLLOWME
	printf("\r\n\r\nHW: Follow-Me");
	#endif
	printf("\r\nFollow Me\r\nSoftware:V%d.%d%c ",VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH + 'a');
	printf("\r\n------------------------------");
	printf("\r\n");


	//BeepTime = 2000;

    Menu_Clear();

	FollowMe_Timer = SetDelay(FOLLOWME_INTERVAL);

	while (1)
	{UBX_Parser();
		// get gps data to update the follow me position
		GPS_Update();

		// update logging
		Logging_Update();

		// check for button action and change state resectively
		if(GetButton())
		{
			BeepTime = 200;

			switch(SysState)
			{
				case STATE_IDLE:
					if(!Error) SysState = STATE_SEND_FOLLOWME; // activate followme only of no error has occured
					break;

				case STATE_SEND_FOLLOWME:
					SysState = STATE_IDLE;
					break;

				default:
					SysState = STATE_IDLE;
					break;
			}

		}

		// state machine
		DebugOut.Analog[9] = SysState;
		switch(SysState)
		{
			case STATE_SEND_FOLLOWME:
				if(CheckDelay(FollowMe_Timer)) // time for next message?
				{
					if(FollowMe.Position.Status == NEWDATA)        // if new
					{   // update remaining data
						FollowMe_Timer = SetDelay(FOLLOWME_INTERVAL);  // reset timer
						FollowMe.Heading = 0;			// invalid heading
						FollowMe.ToleranceRadius = 1;   // 1 meter
						FollowMe.HoldTime = 60;         // go home after 60s without any update
						FollowMe.Event_Flag = 0;        // no event
						FollowMe.Index = 1;             // 2st wp
						FollowMe.reserve[0] = 0;		// reserve
						FollowMe.reserve[1] = 0;		// reserve
						FollowMe.reserve[2] = 0;		// reserve
						FollowMe.reserve[3] = 0;		// reserve
						Request_SendFollowMe = 1;       // triggers serial tranmission

					}
					else // now new position avalable (maybe bad gps signal condition)
					{
						FollowMe_Timer = SetDelay(FOLLOWME_INTERVAL/4);  // reset timer on higer frequency
					}
					LEDGRN_TOGGLE;						// indication of active follow me
				}
				break;

			case STATE_IDLE:
				// do nothing
				LEDGRN_ON;
				break;

			default:
				// triger to idle state
				SysState = STATE_IDLE;
				break;

		}


		// restart ADConversion if ready
		if(ADReady)
		{
			DebugOut.Analog[0] = Adc0;
			DebugOut.Analog[1] = Adc1;
			DebugOut.Analog[2] = Adc2;
			DebugOut.Analog[3] = Adc3;
			DebugOut.Analog[4] = Adc4;
			DebugOut.Analog[5] = Adc5;
			DebugOut.Analog[6] = Adc6;
			DebugOut.Analog[7] = Adc7;

			#ifdef USE_FOLLOWME
			// AVcc = 5V --> 5V = 1024 counts
			// the voltage at the voltage divider reference point is 0.8V less that the UBat
			// because of the silicon diode inbetween.
			// voltage divider R2=10K, R3=3K9
			// UAdc4 = R3/(R3+R2)*UBat= 3.9/(3.9+10)*UBat = UBat/3.564
			UBat = (3 * UBat + (64 * Adc4) / 368) / 4;
			DebugOut.Analog[8] = UBat;

			// check for zellenzahl
			if(PowerOn < 100)
			{
				if(UBat<=84) Zellenzahl = 2;
				else Zellenzahl = 3;
				PowerOn++;
			}
			DebugOut.Analog[16] = Zellenzahl;
			DebugOut.Analog[17] = PowerOn;

			//show recognised Zellenzahl to user
			if(i < Zellenzahl && PowerOn >= 100 && BeepTime == 0 && delay > 1000)
			{
				BeepTime = 100;
				i++;
				delay = 0;
			}
			if(delay < 1500) delay++;

			// monitor battery undervoltage [...||(UBat<74) as temporary workaround to protect 2s lipo packs]
			if(((UBat < Zellenzahl * CELLUNDERVOLTAGE)||(UBat < 74)) && (PowerOn >= 100))
			{   // sound for low battery
				BeepModulation = 0x0300;
				if(!BeepTime)
				{
					BeepTime = 6000; // 0.6 seconds
				}
				Error |= ERROR_LOW_BAT;
			}
			else
			{
				Error &= ~ERROR_LOW_BAT;
			}
			#endif
			ADReady = 0;
			ADC_Enable(); // restart ad conversion sequence
		}

		// serial communication
		USART0_ProcessRxData();
		USART0_TransmitTxData();

		// indicate error, blinking code tbd.
		if(Error)	LEDRED_ON;
		else 		LEDRED_OFF;

    }
 	return (1);
}

