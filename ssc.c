#include <avr/io.h>
#include "ssc.h"

//-------------------------------------- Hardware specific definitions --------------------------------------
#define PORTR_SPI			PINB
#define PORTW_SPI			PORTB		//Port to which the sd-card is connected (SPI Port)
#define PORT_MISO			PORTB6		//Port Pin that is connected to the DO of the MMC/SD-card
#define PORT_MOSI			PORTB5		//Port Pin that is connected to  DI of the MMC/SD-card
#define PORT_SCK			PORTB7		//Port Pin that is connected the CLK of the MMC/SD-card
#define PORT_SS				PORTB4		//Slave Select is not used in SPI Master Mode, but must be defined
#define PORT_CS				PORTB4		//Port Pin that is connected to /CS of the MMC/SD-Karte


#ifdef USE_SDLOGGER
#define	__SD_INTERFACE_INVERTED		// the interface between the controller and the SD-card uses an inverting leveltranslator (transistorinverter)
#endif								// and therefore the signals to or from the memorycard have to be inverted.

#ifdef USE_FOLLOWME					// uses resitors, therefore its not inverted
//#define	__SD_INTERFACE_INVERTED	// the interface between the controller and the MMC/SD-card uses an inverting leveltranslator (transistorinverter)
#endif

#define DDR_SPI				DDRB
#define DD_MISO				DDB6		//Port Pin that is connected to the DO of the MMC/SD-card
#define DD_MOSI				DDB5		//Port Pin that is connected to  DI of the MMC/SD-card
#define DD_SCK				DDB7		//Port Pin that is connected the CLK of the MMC/SD-card
#define DD_SS				DDB4		//Slave Select is not used in SPI Master Mode, but must be defined
#define DD_CS				DDB4		//Port Pin that is connected to /CS of the MMC/SD-Karte

// for compatibility reasons gcc3.x <-> gcc4.x
#ifndef SPCR
#define SPCR   SPCR0
#endif
#ifndef SPIE
#define SPIE   SPIE0
#endif
#ifndef SPE
#define SPE    SPE0
#endif
#ifndef DORD
#define DORD   DORD0
#endif
#ifndef MSTR
#define MSTR   MSTR0
#endif
#ifndef CPOL
#define CPOL   CPOL0
#endif
#ifndef CPHA
#define CPHA   CPHA0
#endif
#ifndef SPR1
#define SPR1   SPR01
#endif
#ifndef SPR0
#define SPR0   SPR00
#endif

#ifndef SPDR
#define SPDR   SPDR0
#endif

#ifndef SPSR
#define SPSR   SPSR0
#endif
#ifndef SPIF
#define SPIF   SPIF0
#endif
#ifndef WCOL
#define WCOL   WCOL0
#endif
#ifndef SPI2X
#define SPI2X  SPI2X0
#endif


//________________________________________________________________________________________________________________________________________
// Function: 	SSC_Init(void);
//
// Description:	This function initialises the synchronus serial channel to the sdcard.
//
//
// Returnvalue: none
//________________________________________________________________________________________________________________________________________

void SSC_Init(void)
{
	// Set MOSI,SCK and CS as output
	DDR_SPI |= (1<<DD_MOSI)|(1<<DD_SCK)|(1<<DD_CS);
	// set MISO as input
	DDR_SPI &= ~(1<<DD_MISO);

	SSC_Disable();

	// 20MHz / 32 = 625 kHz
	#ifdef __SD_INTERFACE_INVERTED
	SPCR = (1<<SPE)|(1<<MSTR)|(0<<DORD)|(1<<CPOL)|(0<<CPHA)|(1<<SPR1)|(0<<SPR0);    // Enable SSC in mastermode, inverted clockpolarity (idle high)
	#else
	SPCR = (1<<SPE)|(1<<MSTR)|(0<<DORD)|(0<<CPOL)|(0<<CPHA)|(1<<SPR1)|(0<<SPR0); 	// Enable SSC in mastermode, noninverted clockpolarity (idle low)
	#endif
	SPSR |= (1<<SPI2X);

	// set port pin as input pullup for SD-Card switch
	#ifdef USE_FOLLOWME
	PORTB |= (1 << PORTB2);
	DDRB &= ~(1 << DDB2);
	#endif

	#ifdef USE_SDLOGGER
	PORTB |= (1 << PORTB3);
	DDRB &= ~(1 << DDB3);
	#endif
}

void	SSC_Deinit(void)
{
	SSC_Disable();
	SPCR = 0;
	SPSR = 0;
}

//________________________________________________________________________________________________________________________________________
// Function: 	SSC_GetChar(void);
//
// Description:	This function reads one byte from the SSC
//
//
// Returnvalue: the byte received.
//________________________________________________________________________________________________________________________________________

uint8_t SSC_GetChar (void)
{
	uint8_t Byte = 0;

	#ifdef __SD_INTERFACE_INVERTED
	SPDR = 0x00;										// send dummy byte to initiate the reading
	#else
	SPDR = 0xFF;										// send dummy byte to initiate the reading
	#endif
	while(!(SPSR & (1<<SPIF)))
	{
		// wait until the data has been read.
	}
	Byte = SPDR;

	#ifdef __SD_INTERFACE_INVERTED
	Byte = ~Byte;
	#endif

	return(Byte);
}


//________________________________________________________________________________________________________________________________________
// Function: 	SSC_PutChar(u8 Byte);
//
// Description:	This function writes one byte to the SSC
//
//
// Returnvalue: none
//________________________________________________________________________________________________________________________________________

void SSC_PutChar (uint8_t Byte)
{

	#ifdef __SD_INTERFACE_INVERTED
	SPDR = ~Byte; 										// send one byte of data to the SSC
	#else
	SPDR =  Byte; 										// send one byte of data to the SSC
	#endif
	while(!(SPSR & (1<<SPIF)))
	{
		// wait until the data has been sent.
	}
}


//________________________________________________________________________________________________________________________________________
// Function: 	SSC_Disable(void);
//
// Description:	This function enables chipselect of the sdcard (active low)
//
//
// Returnvalue: none
//________________________________________________________________________________________________________________________________________

void SSC_Disable(void)
{
	#ifdef __SD_INTERFACE_INVERTED
	PORTW_SPI &= ~(1<<PORT_CS);					// disable chipselect of the sdcard (active low).
	#else
	PORTW_SPI |= (1<<PORT_CS); 					// disable chipselect of the sdcard (active low).
	#endif
}




//________________________________________________________________________________________________________________________________________
// Function: 	SSC_Enable(void);
//
// Description:	This function disables chipselect of the sdcard (active low)
//
//
// Returnvalue: none
//________________________________________________________________________________________________________________________________________

void SSC_Enable(void)
{
	#ifdef __SD_INTERFACE_INVERTED
	PORTW_SPI |= (1<<PORT_CS); 					// enable chipselect of the sdcard (active low).
	#else
	PORTW_SPI &= ~(1<<PORT_CS);					// enable chipselect of the sdcard (active low).
	#endif
}


