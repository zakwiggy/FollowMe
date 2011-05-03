 // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// + Copyright (c) 04.2007 Holger Buss
// + only for non-profit use
// + www.MikroKopter.com
// + see the File "License.txt" for further Informations
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <stdarg.h>
#include <string.h>

#include "main.h"
#include "menu.h"
#include "timer0.h"
#include "uart0.h"
#include "ubx.h"
#include "printf_P.h"


#define FC_ADDRESS 1
#define NC_ADDRESS 2
#define MK3MAG_ADDRESS 3

#define FM_ADDRESS 10 // FOLLOW ME

#define FALSE	0
#define TRUE	1

#define ABO_TIMEOUT 4000 // disable abo after 4 seconds

//int8_t test __attribute__ ((section (".noinit")));
uint8_t Request_VerInfo 		= FALSE;
uint8_t Request_Display 		= FALSE;
uint8_t Request_Display1 		= FALSE;
uint8_t Request_ExternalControl = FALSE;
uint8_t Request_DebugData 		= FALSE;
uint8_t Request_DebugLabel 		= 255;
uint8_t Request_SendFollowMe	= FALSE;
uint8_t DisplayLine = 0;
uint8_t DisplayKeys = 0;

volatile uint8_t txd_buffer[TXD_BUFFER_LEN];
volatile uint8_t rxd_buffer_locked = FALSE;
volatile uint8_t rxd_buffer[RXD_BUFFER_LEN];
volatile uint8_t txd_complete = TRUE;
volatile uint8_t ReceivedBytes = 0;
volatile uint8_t *pRxData = 0;
volatile uint8_t RxDataLen = 0;

uint8_t PcAccess = 100;
uint16_t AboTimeOut = 0;

ExternControl_t ExternControl;
DebugOut_t		DebugOut;
UART_VersionInfo_t	UART_VersionInfo;

uint16_t DebugData_Timer;
uint16_t DebugData_Interval = 0; // in 1ms
uint16_t Display_Timer;
uint16_t Display_Interval = 0;

Waypoint_t FollowMe;

// keep lables in flash to save 512 bytes of sram space
const prog_uint8_t ANALOG_LABEL[32][16] =
{
   //1234567890123456
    "Analog_Ch0      ", //0
    "Analog_Ch1      ",
    "Analog_Ch2      ",
    "Analog_Ch3      ",
    "Analog_Ch4      ",
    "Analog_Ch5      ", //5
    "Analog_Ch6      ",
    "Analog_Ch7      ",
    "UBat            ",
    "SysState        ",
    "Debug10         ", //10
    "Debug11         ",
    "Debug12         ",
    "Debug13         ",
    "Debug14         ",
    "Debug15         ", //15
	"Zellenzahl      ",
    "PowerOn         ",
    "Debug18         ",
    "Debug19         ",
    "Debug20         ", //20
    "Debug21         ",
    "Debug22         ",
    "Debug23         ",
    "Debug24         ",
    "Debug25         ", //25
    "Debug26         ",
    "Debug27         ",
    "Debug28         ",
    "Debug29         ",
    "Debug30         ", //30
    "Debug31         "
};



/****************************************************************/
/*              Initialization of the USART0                    */
/****************************************************************/
void USART0_Init (void)
{
	uint8_t sreg = SREG;
	uint16_t ubrr = (uint16_t) ((uint32_t) SYSCLK/(8 * USART0_BAUD) - 1);

	// disable all interrupts before configuration
	cli();

	// disable RX-Interrupt
	UCSR0B &= ~(1 << RXCIE0);
	// disable TX-Interrupt
	UCSR0B &= ~(1 << TXCIE0);

	// set direction of RXD0 and TXD0 pins
	// set RXD0 (PD0) as an input pin
	PORTD |= (1 << PORTD0);
	DDRD &= ~(1 << DDD0);
	// set TXD0 (PD1) as an output pin
	PORTD |= (1 << PORTD1);
	DDRD |=  (1 << DDD1);

	// USART0 Baud Rate Register
	// set clock divider
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;

   	// USART0 Control and Status Register A, B, C

	// enable double speed operation in
	UCSR0A |= (1 << U2X0);
	// enable receiver and transmitter in
	UCSR0B = (1 << TXEN0) | (1 << RXEN0);
	// set asynchronous mode
	UCSR0C &= ~(1 << UMSEL01);
	UCSR0C &= ~(1 << UMSEL00);
	// no parity
	UCSR0C &= ~(1 << UPM01);
	UCSR0C &= ~(1 << UPM00);
	// 1 stop bit
	UCSR0C &= ~(1 << USBS0);
	// 8-bit
	UCSR0B &= ~(1 << UCSZ02);
	UCSR0C |=  (1 << UCSZ01);
	UCSR0C |=  (1 << UCSZ00);

		// flush receive buffer
	while ( UCSR0A & (1<<RXC0) ) UDR0;

	// enable interrupts at the end
	// enable RX-Interrupt
	UCSR0B |= (1 << RXCIE0);
	// enable TX-Interrupt
	UCSR0B |= (1 << TXCIE0);

	// initialize the debug timer
	DebugData_Timer = SetDelay(DebugData_Interval);

	// unlock rxd_buffer
	rxd_buffer_locked = FALSE;
	pRxData = 0;
	RxDataLen = 0;

	// no bytes to send
	txd_complete = TRUE;

	UART_VersionInfo.SWMajor = VERSION_MAJOR;
	UART_VersionInfo.SWMinor = VERSION_MINOR;
	UART_VersionInfo.SWPatch = VERSION_PATCH;
	UART_VersionInfo.ProtoMajor = VERSION_SERIAL_MAJOR;
	UART_VersionInfo.ProtoMinor = VERSION_SERIAL_MINOR;

	// restore global interrupt flags
    SREG = sreg;
	sei();
    printf("\r\n UART0 init...ok");
}

/****************************************************************/
/*               USART0 transmitter ISR                         */
/****************************************************************/
ISR(USART0_TX_vect)
{
	static uint16_t ptr_txd_buffer = 0;
	uint8_t tmp_tx;
	if(!txd_complete) // transmission not completed
	{
		ptr_txd_buffer++;                    // die [0] wurde schon gesendet
		tmp_tx = txd_buffer[ptr_txd_buffer];
		// if terminating character or end of txd buffer was reached
		if((tmp_tx == '\r') || (ptr_txd_buffer == TXD_BUFFER_LEN))
		{
			ptr_txd_buffer = 0; // reset txd pointer
			txd_complete = 1; // stop transmission
		}
		UDR0 = tmp_tx; // send current byte will trigger this ISR again
	}
	// transmission completed
	else ptr_txd_buffer = 0;
}

/****************************************************************/
/*               USART0 receiver ISR                            */
/****************************************************************/
ISR(USART0_RX_vect)
{
	static uint16_t crc;
	static uint8_t ptr_rxd_buffer = 0;
	uint8_t crc1, crc2;
	uint8_t c;

	c = UDR0;  // catch the received byte

	if(rxd_buffer_locked) return; // if rxd buffer is locked immediately return

	// the rxd buffer is unlocked
	if((ptr_rxd_buffer == 0) && (c == '#')) // if rxd buffer is empty and syncronisation character is received
	{
		rxd_buffer[ptr_rxd_buffer++] = c; // copy 1st byte to buffer
		crc = c; // init crc
	}
	#if 0
	else if (ptr_rxd_buffer == 1) // handle address
	{
		rxd_buffer[ptr_rxd_buffer++] = c; // copy byte to rxd buffer
		crc += c; // update crc
	}
	#endif
	else if (ptr_rxd_buffer < RXD_BUFFER_LEN) // collect incomming bytes
	{
		if(c != '\r') // no termination character
		{
			rxd_buffer[ptr_rxd_buffer++] = c; // copy byte to rxd buffer
			crc += c; // update crc
		}
		else // termination character was received
		{
			// the last 2 bytes are no subject for checksum calculation
			// they are the checksum itself
			crc -= rxd_buffer[ptr_rxd_buffer-2];
			crc -= rxd_buffer[ptr_rxd_buffer-1];
			// calculate checksum from transmitted data
			crc %= 4096;
			crc1 = '=' + crc / 64;
			crc2 = '=' + crc % 64;
			// compare checksum to transmitted checksum bytes
			if((crc1 == rxd_buffer[ptr_rxd_buffer-2]) && (crc2 == rxd_buffer[ptr_rxd_buffer-1]))
			{   // checksum valid
				rxd_buffer[ptr_rxd_buffer] = '\r'; // set termination character
				ReceivedBytes = ptr_rxd_buffer + 1;// store number of received bytes
				rxd_buffer_locked = TRUE;          // lock the rxd buffer
				// if 2nd byte is an 'R' enable watchdog that will result in an reset
				if(rxd_buffer[2] == 'R') {wdt_enable(WDTO_250MS);} // Reset-Commando
			}
			else
			{	// checksum invalid
				rxd_buffer_locked = FALSE; // unlock rxd buffer
			}
			ptr_rxd_buffer = 0; // reset rxd buffer pointer
		}
	}
	else // rxd buffer overrun
	{
		ptr_rxd_buffer = 0; // reset rxd buffer
		rxd_buffer_locked = FALSE; // unlock rxd buffer
	}

}


// --------------------------------------------------------------------------
void AddCRC(uint16_t datalen)
{
	uint16_t tmpCRC = 0, i;
	for(i = 0; i < datalen; i++)
	{
		tmpCRC += txd_buffer[i];
	}
	tmpCRC %= 4096;
	txd_buffer[i++] = '=' + tmpCRC / 64;
	txd_buffer[i++] = '=' + tmpCRC % 64;
	txd_buffer[i++] = '\r';
	txd_complete = FALSE;
	UDR0 = txd_buffer[0]; // initiates the transmittion (continued in the TXD ISR)
}



// --------------------------------------------------------------------------
void SendOutData(uint8_t cmd, uint8_t addr, uint8_t numofbuffers, ...) // uint8_t *pdata, uint8_t len, ...
{
	va_list ap;
	uint16_t pt = 0;
	uint8_t a,b,c;
	uint8_t ptr = 0;

	uint8_t *pdata = 0;
	int len = 0;

	txd_buffer[pt++] = '#';			// Start character
	txd_buffer[pt++] = 'a' + addr;	// Address (a=0; b=1,...)
	txd_buffer[pt++] = cmd;			// Command

	va_start(ap, numofbuffers);
	if(numofbuffers)
	{
		pdata = va_arg(ap, uint8_t*);
		len = va_arg(ap, int);
		ptr = 0;
		numofbuffers--;
	}

	while(len)
	{
		if(len)
		{
			a = pdata[ptr++];
			len--;
			if((!len) && numofbuffers)
			{
				pdata = va_arg(ap, uint8_t*);
				len = va_arg(ap, int);
				ptr = 0;
				numofbuffers--;
			}
		}
		else a = 0;
		if(len)
		{
			b = pdata[ptr++];
			len--;
			if((!len) && numofbuffers)
			{
				pdata = va_arg(ap, uint8_t*);
				len = va_arg(ap, int);
				ptr = 0;
				numofbuffers--;
			}
		}
		else b = 0;
		if(len)
		{
			c = pdata[ptr++];
			len--;
			if((!len) && numofbuffers)
			{
				pdata = va_arg(ap, uint8_t*);
				len = va_arg(ap, int);
				ptr = 0;
				numofbuffers--;
			}
		}
		else c = 0;
		txd_buffer[pt++] = '=' + (a >> 2);
		txd_buffer[pt++] = '=' + (((a & 0x03) << 4) | ((b & 0xf0) >> 4));
		txd_buffer[pt++] = '=' + (((b & 0x0f) << 2) | ((c & 0xc0) >> 6));
		txd_buffer[pt++] = '=' + ( c & 0x3f);
	}
	va_end(ap);
	AddCRC(pt); // add checksum after data block and initates the transmission
}


// --------------------------------------------------------------------------
void Decode64(void)
{
	uint8_t a,b,c,d;
	uint8_t x,y,z;
	uint8_t ptrIn = 3;
	uint8_t ptrOut = 3;
	uint8_t len = ReceivedBytes - 6;

	while(len)
	{
		a = rxd_buffer[ptrIn++] - '=';
		b = rxd_buffer[ptrIn++] - '=';
		c = rxd_buffer[ptrIn++] - '=';
		d = rxd_buffer[ptrIn++] - '=';
		//if(ptrIn > ReceivedBytes - 3) break;

		x = (a << 2) | (b >> 4);
		y = ((b & 0x0f) << 4) | (c >> 2);
		z = ((c & 0x03) << 6) | d;

		if(len--) rxd_buffer[ptrOut++] = x; else break;
		if(len--) rxd_buffer[ptrOut++] = y; else break;
		if(len--) rxd_buffer[ptrOut++] = z; else break;
	}
	pRxData = &rxd_buffer[3];
	RxDataLen = ptrOut - 3;
}


// --------------------------------------------------------------------------
void USART0_ProcessRxData(void)
{
	// if data in the rxd buffer are not locked immediately return
	if(!rxd_buffer_locked) return;

	Decode64(); // decode data block in rxd_buffer


	switch(rxd_buffer[1] - 'a')
	{
		case FM_ADDRESS:

			switch(rxd_buffer[2])
			{
				default:
					//unsupported command received
					break;
			} // case FC_ADDRESS:

		default: // any Slave Address

		switch(rxd_buffer[2])
			{
				case 'a':// request for labels of the analog debug outputs
					Request_DebugLabel = pRxData[0];
					if(Request_DebugLabel > 31) Request_DebugLabel = 31;
					PcAccess = 255;
					break;

				case 'h':// request for display columns
					PcAccess = 255;
					if((pRxData[0] & 0x80) == 0x00) // old format
					{
						DisplayLine = 2;
						Display_Interval = 0;
					}
					else // new format
					{
						DisplayKeys |= ~pRxData[0];
						Display_Interval = (uint16_t) pRxData[1] * 10;
						DisplayLine = 4;
						AboTimeOut = SetDelay(ABO_TIMEOUT);
					}
					Request_Display = TRUE;
					break;

				case 'l':// request for display columns
					PcAccess = 255;
					MenuItem = pRxData[0];
					Request_Display1 = TRUE;
				break;

				case 'v': // request for version and board release
					Request_VerInfo = TRUE;
					break;

				case 'd': // request for the debug data
					DebugData_Interval = (uint16_t) pRxData[0] * 10;
					if(DebugData_Interval > 0) Request_DebugData = TRUE;
					AboTimeOut = SetDelay(ABO_TIMEOUT);
				break;

				case 'g':// get external control data
					Request_ExternalControl = TRUE;
					break;

				default:
					//unsupported command received
					break;
		}
		break; // default:
	}
	// unlock the rxd buffer after processing
	pRxData = 0;
	RxDataLen = 0;
	rxd_buffer_locked = FALSE;
}

//############################################################################
//Routine für die Serielle Ausgabe
int16_t uart_putchar (int8_t c)
//############################################################################
{
	if (c == '\n')
		uart_putchar('\r');
	// wait until previous character was send
	loop_until_bit_is_set(UCSR0A, UDRE0);
	// send character
	UDR0 = c;
	return (0);
}


//---------------------------------------------------------------------------------------------
void USART0_TransmitTxData(void)
{
	if(!txd_complete) return;

	if(CheckDelay(AboTimeOut))
	{
		Display_Interval = 0;
		DebugData_Interval = 0;
	}

	if(Request_VerInfo && txd_complete)
	{
		SendOutData('V', FM_ADDRESS, 1, (uint8_t *) &UART_VersionInfo, sizeof(UART_VersionInfo));
		Request_VerInfo = FALSE;
	}
	else if( (((Display_Interval > 0) && CheckDelay(Display_Timer)) || Request_Display) && txd_complete)
	{
		if(DisplayLine > 3)// new format
		{
			Menu_Update(DisplayKeys);
			DisplayKeys = 0;
			SendOutData('H', FC_ADDRESS, 1, (uint8_t *)DisplayBuff, sizeof(DisplayBuff));
		}
		else // old format
		{
			//LCD_printfxy(0,0,"!!! INCOMPATIBLE !!!");
			SendOutData('H', FC_ADDRESS, 2, &DisplayLine, sizeof(DisplayLine), (uint8_t *)DisplayBuff, 20);
			if(DisplayLine++ > 3) DisplayLine = 0;
		}
		Display_Timer = SetDelay(Display_Interval);
		Request_Display = FALSE;
	}
	else if(Request_Display1 && txd_complete)
	{
		Menu_Update(0);
		SendOutData('L', FC_ADDRESS, 3, &MenuItem, sizeof(MenuItem), &MaxMenuItem, sizeof(MaxMenuItem), DisplayBuff, sizeof(DisplayBuff));
		Request_Display1 = FALSE;
	}
	else if(Request_DebugLabel != 0xFF) // Texte für die Analogdaten
	{
		uint8_t label[16]; // local sram buffer
		memcpy_P(label, ANALOG_LABEL[Request_DebugLabel], 16); // read lable from flash to sram buffer
		SendOutData('A', FM_ADDRESS, 2, (uint8_t *) &Request_DebugLabel, sizeof(Request_DebugLabel), label, 16);
		Request_DebugLabel = 0xFF;
	}
	else if(Request_ExternalControl && txd_complete)
	{
		SendOutData('G', FM_ADDRESS, 1,(uint8_t *) &ExternControl, sizeof(ExternControl));
		Request_ExternalControl = FALSE;
	}
	else if( (((DebugData_Interval > 0) && CheckDelay(DebugData_Timer)) || Request_DebugData) && txd_complete)
	{
		SendOutData('D', FM_ADDRESS, 1,(uint8_t *) &DebugOut, sizeof(DebugOut));
		DebugData_Timer = SetDelay(DebugData_Interval);
		Request_DebugData = FALSE;
    }
	else if(Request_SendFollowMe && txd_complete)
	{
		SendOutData('s', NC_ADDRESS, 1, (uint8_t *)&FollowMe, sizeof(FollowMe));
		FollowMe.Position.Status = PROCESSED;
		Request_SendFollowMe = FALSE;
	}
}

