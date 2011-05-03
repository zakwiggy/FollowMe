#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "sdc.h"
#include "ssc.h"
#include "timer0.h"
#include "printf_P.h"
#include "crc16.h"

//#define _SD_DEBUG

#define CMD_GO_IDLE_STATE		0x00	/* CMD00: response R1 */
#define CMD_SEND_OP_COND		0x01 	/* CMD01: response R1 */
#define CMD_SEND_IF_COND		0x08	/* CMD08: response R7 */
#define CMD_SEND_CSD			0x09	/* CMD09: response R1 */
#define CMD_SEND_CID			0x0A 	/* CMD10: response R1 */
#define CMD_SEND_STATUS			0x0D	/* CMD13: response R2 */
#define CMD_SET_BLOCKLEN		0x10	/* CMD16: arg0[31:0]: block length, response R1*/
#define CMD_READ_SINGLE_BLOCK 	0x11	/* CMD17: arg0[31:0]: data address, response R1 */
#define CMD_WRITE_SINGLE_BLOCK	0x18	/* CMD24: arg0[31:0]: data address, response R1 */
#define CMD_APP_CMD				0x37	/* CMD55: response R1 */
#define CMD_READ_OCR 			0x3A 	/* CMD58: response R3 */
#define CMD_CRC_ON_OFF 			0x3B 	/* CMD59: arg0[31:1]: stuff bits, arg0[0:0]: crc option, response R1 */
#define ACMD_SEND_OP_COND		0x29	/* ACMD41: arg0[31]: stuff bits, arg0[30]: HCS, arg0[29:0] stuff bits*, response R1 */

#define R1_NO_ERR	 			0x00
#define R1_IDLE_STATE 			0x01
#define R1_ERASE_RESET 			0x02
#define R1_ILLEGAL_CMD 			0x04
#define R1_COM_CRC_ERR	 		0x08
#define R1_ERASE_SEQUENCE_ERR	0x10
#define R1_ADDRESS_ERR 			0x20
#define R1_PARAMETER_ERR 		0x40
#define R1_BAD_RESPONSE 		0x80

#define R2_NO_ERR	 			0x00
#define R2_CARD_LOCKED 			0x01
#define R2_ERASE_WRITE_PROT_ERR	0x02
#define R2_UNKOWN_ERR			0x04
#define R2_CARD_CTRL_ERR 		0x08
#define R2_CARD_ECC_ERR		    0x10
#define R2_WRITE_PROT_ERR		0x20
#define R2_ERASE_PARAM_ERR 		0x40
#define R2_OUT_OF_RANGE_ERR		0x80

#define DATA_START_TOKEN 		0xFE
#define DATA_RESPONSE_MASK 		0x1F
#define DATA_RESPONSE_OK 		0x05
#define DATA_RESPONSE_CRC_ERR	0x0B
#define DATA_RESPONSE_WRITE_ERR 0x1D

typedef enum
{
	VER_UNKNOWN,
 	VER_1X,
	VER_20
} SDVersion_t;

typedef struct
{
  uint8_t Valid;
  SDVersion_t Version;  // HW-Version
  uint32_t Capacity;			// Memory capacity in bytes
  uint8_t CID[16];			// CID register
  uint8_t CSD[16];			// CSD register
} __attribute__((packed)) SDCardInfo_t;


volatile SDCardInfo_t SDCardInfo;

//________________________________________________________________________________________________________________________________________
// Function: 	CRC7(uint8_t* cmd, uint32_t len);
//
// Description:	This function calculated the CRC7 checksum used in the last byte of a spi command frame.
//
//
// Returnvalue: the function returns the crc7 including bit 0 set to 1
//________________________________________________________________________________________________________________________________________

uint8_t CRC7(uint8_t *cmd, uint32_t len)
{
	uint8_t i, a;
	uint8_t crc, Data;

	crc = 0; // init CRC buffer
	for (a = 0; a < len ;a++) // for every byte in the msg
	{
		Data = cmd[a];
		for (i=0;i<8;i++) // for every bit in the byte
		{
			crc <<= 1; // shift crc
			if ((Data & 0x80)^(crc & 0x80))	crc ^=0x09;	  //xor
			Data <<= 1;	// shift data  for next bit
		}
	}
	crc = (crc<<1)|1; // set terminating bit to 1
	return(crc);
}


uint8_t SDC_WaitForBusy(uint16_t timeout)
{
	uint8_t rsp = 0;
	uint16_t timestamp = 0;

	SSC_Enable();				// enable chipselect.
	timestamp = SetDelay(timeout);
	do
	{
		rsp = SSC_GetChar();
		if(CheckDelay(timestamp)) break;
	}while(rsp != 0xFF);		// wait while card is busy (data out low)
	return(rsp);
}

//________________________________________________________________________________________________________________________________________
// Function: 	SDC_SendCMDR1(uint8_t CmdNo, uint32_t arg);
//
// Description:	This function send a command frame to the SD-Card in spi-mode.
//
//
// Returnvalue: The function returns the first response byte like for R1 commands
//________________________________________________________________________________________________________________________________________
uint8_t SDC_SendCMDR1(uint8_t CmdNo, uint32_t arg)
{
	uint8_t r1;
	uint16_t timeout = 0;
	uint16_t a;
	uint8_t cmd[6];

	cmd[0] = 0x40|CmdNo; 	// set command index
	cmd[1] = (arg & 0xFF000000)>>24;
	cmd[2] = (arg & 0x00FF0000)>>16;
	cmd[3] = (arg & 0x0000FF00)>>8;
   	cmd[4] = (arg & 0x000000FF);
	cmd[5] = CRC7(cmd, 5); // update checksum
	#ifdef _SD_DEBUG
	printf("\r\nCmd=%02X, arg=%04X%04X", CmdNo, (uint16_t)(arg>>16), (uint16_t)(0xFFFF & arg));
	#endif
	SSC_Disable();			// disable chipselect.
	SSC_PutChar(0xFF);      // dummy to sync
	SSC_Enable();			// enable chipselect.

	SDC_WaitForBusy(500);	// wait 500ms until card is busy

	for (a = 0;a < 6; a++) // send the command sequence to the sdcard (6 bytes)
	{
		SSC_PutChar(cmd[a]);
		_delay_loop_2(10);
	}
	// get response byte
	do
	{
		r1 = SSC_GetChar();       // get byte from sd-card
		if (timeout++ > 500) break;
	}while(r1 == 0xFF); // wait for the response byte from sd-card.
	#ifdef _SD_DEBUG
	printf("-->R1=%02X", r1);
	#endif
	return(r1);
}

//________________________________________________________________________________________________________________________________________
// Function: 	SDC_SendACMDR1(uint8_t CmdNo, uint32_t arg);
//
// Description:	This function send a application command frame to the SD-Card in spi-mode.
//
//
// Returnvalue: The function returns the first response byte like for R1 commands
//________________________________________________________________________________________________________________________________________
uint8_t SDC_SendACMDR1(uint8_t CmdNo, uint32_t arg)
{
  	uint8_t r1 = 0xFF;
	r1 = SDC_SendCMDR1(CMD_APP_CMD, 0UL);
	if(r1 & R1_BAD_RESPONSE) return(r1);
	r1 = SDC_SendCMDR1(CmdNo, arg);
	return(r1);
}

//________________________________________________________________________________________________________________________________________
// Function: 	SDC_GetData(uint8_t * cmd ,u8 *Buffer, u32 len);
//
// Description:	This function sneds cmd an reads a datablock of len from the sd-card
//
//
// Returnvalue: SD_Result_t
//________________________________________________________________________________________________________________________________________

SD_Result_t SDC_GetData(uint8_t CmdNo, uint32_t addr, uint8_t *Buffer, uint32_t len)
{
	uint8_t rsp;
	uint16_t a, crc16;
	SD_Result_t result = SD_ERROR_UNKNOWN;

	// send the command
	rsp = SDC_SendCMDR1(CmdNo, addr);
	if (rsp != R1_NO_ERR)
	{
		result = SD_ERROR_BAD_RESPONSE;
		goto end;
	}

	do
	{
		rsp = SSC_GetChar();
		if((rsp & 0xF0) == 0x00) // data error token
		{
			result = SD_ERROR_READ_DATA;
			goto end;
		}
	}while(rsp != DATA_START_TOKEN);
	// data start token received
	for (a = 0; a < len; a++)	// read the block from the SSC
	{
		Buffer[a] = SSC_GetChar();
	}
	// Read two bytes CRC16-Data checksum
	crc16 = SSC_GetChar(); // highbyte first
	crc16 = (crc16<<8)|SSC_GetChar(); // lowbyte last
/*	if(crc16 != CRC16(Buffer, len)) result = SD_ERROR_CRC_DATA;
	else */result = SD_SUCCESS;

	end:
	if(result != SD_SUCCESS)
	{
	 	printf("Error %02X reading data from sd card (R1=%02X).\r\n", result, rsp);
	}
	return(result);
}


//________________________________________________________________________________________________________________________________________
// Function: 	SDC_PrintCID(u8 * pCID);
//
// Description:	This function prints the CIS register in a human readable format.
//
//
// Returnvalue: the function returns nothing
//________________________________________________________________________________________________________________________________________

void SDC_PrintCID(uint8_t * pCID)
{
	uint8_t pn[6];
	uint16_t temp1, temp2;

	printf("\r\n  Manufacturer ID: %i\r\n", pCID[0]);
	memcpy(pn, &pCID[1], 2);
	pn[2] = '\0'; // terminate string
	printf("  Application ID: %s\r\n",pn);
	memcpy(pn, &pCID[3], 5);
	pn[5] = '\0'; // terminate string
	printf("  Product Name: %s\r\n",pn);
	printf("  Product Rev.: %i.%i\r\n",pCID[8]>>4, pCID[8]&0xF);
	printf("  Serial No.: ");
	for(temp1 = 0; temp1<4; temp1++)
	{
		printf("%02X", pCID[9+temp1]);
	}
	printf("\r\n");
	temp1 = pCID[14] & 0x0F;    // month
	temp2 = ((pCID[14]>>4)|(pCID[13]<<4)) + 2000; // year
	printf("  Manufac. Date: %i/%i\r\n\r\n",temp1, temp2);
}

//________________________________________________________________________________________________________________________________________
// Function: 	SDC_GetCID(uint8_t * pCID);
//
// Description:	This function reads the CIS register form the sd card in spi mode.
//
//
// Returnvalue: the function returns error state
//________________________________________________________________________________________________________________________________________

SD_Result_t SDC_GetCID(uint8_t * pCID)
{
	return SDC_GetData(CMD_SEND_CID, 0UL, pCID, 16);
}

//________________________________________________________________________________________________________________________________________
// Function: 	SDC_GetCSD(uint8_t * pCSD);
//
// Description:	This function reads the CSD register form the sd card in spi mode.
//
//
// Returnvalue: the function returns error state
//________________________________________________________________________________________________________________________________________

SD_Result_t SDC_GetCSD(uint8_t * pCSD)
{
	return SDC_GetData(CMD_SEND_CSD, 0UL, pCSD, 16);
}


//________________________________________________________________________________________________________________________________________
// Function: 	SDC_Init(void);
//
// Description:	This function initialises the SDCard to spi-mode.
//
//
// Returnvalue: the function returns 0 if the initialisation was successfull otherwise the function returns an errorcode.
//________________________________________________________________________________________________________________________________________

SD_Result_t SDC_Init(void)
{
	uint16_t timeout = 0;
	uint8_t rsp[6]; // SD-SPI response buffer
	SD_Result_t result = SD_ERROR_UNKNOWN;

	if(SD_SWITCH) // init only if the SD-Switch is indicating a card in the slot
	{
		printf("\r\n SSC init...");
		SSC_Init();
		printf("ok");

		//_delay_loop_2(1050);

		printf("\r\n SDC init...");
		SDCardInfo.Valid = 0;
		/* The host shall supply power to the card so that the voltage is reached to Vdd_min within 250ms and
		start to supply at least 74 SD clocks to the SD card with keeping cmd line to high. In case of SPI
		mode, CS shall be held to high during 74 clock cycles. */
		SSC_Disable(); // set SD_CS high
		for (timeout = 0; timeout < 15; timeout++) 	// 15*8 = 120 cycles
		{
			SSC_PutChar(0xFF);
		}

		// switch to idle state
		#ifdef _SD_DEBUG
		printf("\r\nGoing idle state..");
		#endif
		timeout = 0;
		do
		{
			rsp[0] = SDC_SendCMDR1(CMD_GO_IDLE_STATE, 0UL);
			if (timeout++ > 500)
			{
				printf("reset timeout");
				result = SD_ERROR_RESET;
				goto end;
			}
		}while(rsp[0] != R1_IDLE_STATE);
	    // enable crc feature
/*		if(SDC_SendCMDR1(CMD_CRC_ON_OFF, 1UL) != R1_IDLE_STATE)
		{
				printf("Bad cmd59 R1=%02X.", rsp[0]);
				result = SD_ERROR_BAD_RESPONSE;
				goto end;
		}*/
		// check for card hw version
		// 2.7-3.6V Range = 0x01, check pattern 0xAA
		rsp[0] = SDC_SendCMDR1(CMD_SEND_IF_COND, 0x000001AA);
		// answer to cmd58 is an R7 response (R1+ 4Byte IFCond)
		if(rsp[0] & R1_BAD_RESPONSE)
		{
			printf("Bad cmd8 R1=%02X.", rsp[0]);
			result = SD_ERROR_BAD_RESPONSE;
			goto end;
		}
		if(rsp[0] & R1_ILLEGAL_CMD)
		{
			//Ver1.X SD Memory Card or not a SD Memory Card
			SDCardInfo.Version = VER_1X;
		}
		else
		{
		   // Ver2.00 or later SD Memory Card
		   // reading the remaining bytes of the R7 response
		   SDCardInfo.Version = VER_20;
		   for(timeout = 1; timeout < 5; timeout++)
		   {
				rsp[timeout] = SSC_GetChar();
		   }
		   //check pattern
		   if(rsp[4]!= 0xAA)
		   {
			 	printf("Bad cmd8 R7 check pattern.\r\n");
				result = SD_ERROR_BAD_RESPONSE;
				goto end;
		   }
		   if ( (rsp[3] & 0x0F)!= 0x01 ) // voltage range is not 2.7-3.6V
		   {

		   		printf("Card is incompatible to 3.3V.\r\n");
				result = SD_ERROR_BAD_VOLTAGE_RANGE;
				goto end;
		   }
		}

		rsp[0] = SDC_SendCMDR1(CMD_READ_OCR, 0UL);
		// answer to cmd58 is an R3 response (R1 + 4Byte OCR)
		if(rsp[0] & R1_BAD_RESPONSE)
		{
			printf("Bad cmd58 R1 %02x.", rsp[0]);
			result = SD_ERROR_BAD_RESPONSE;
			goto end;
		}
		if(rsp[0] & R1_ILLEGAL_CMD)
		{
			printf("Not an SD-CARD.");
			result = SD_ERROR_NO_SDCARD;
			goto end;
		}
		// read 4 bytes of OCR register
		for(timeout = 1; timeout < 5; timeout++)
		{
			rsp[timeout] = SSC_GetChar();
		}
		//	FollowMe & SD-Logger uses 3.3 V,  therefore check for bit 20 & 21
		if((rsp[2] & 0x30) != 0x30)
		{
		 	// supply voltage is not supported by sd-card
			printf("Card is incompatible to 3.3V.");
			result = SD_ERROR_BAD_VOLTAGE_RANGE;
			goto end;
		}

		// Initialize the sd-card sending continously ACMD_SEND_OP_COND (only supported by SD cards)
		timeout =  SetDelay(2000); // set timeout to 2000 ms (large cards tend to longer)
		do
		{
			rsp[0] = SDC_SendACMDR1(ACMD_SEND_OP_COND, 0UL);
			if(rsp[0] & R1_BAD_RESPONSE)
			{
				printf("Bad Acmd41 R1=%02X.", rsp[0]);
				result = SD_ERROR_BAD_RESPONSE;
				goto end;
			}
			if(CheckDelay(timeout))
			{
			    printf("Init timeout.");
				result = SD_ERROR_INITIALIZE;
				goto end;
			}
		} while(rsp[0] & R1_IDLE_STATE); // loop until idle state

		if(rsp[0] != R1_NO_ERR)
		{
			printf("Init error.");
		 	result = SD_ERROR_INITIALIZE;
			goto end;
		}
		/* set block size to 512 bytes */
    	if(SDC_SendCMDR1(CMD_SET_BLOCKLEN, 512UL) != R1_NO_ERR)
    	{
        	printf("Error setting block length to 512.");
		 	result = SD_ERROR_SET_BLOCKLEN;
			goto end;
    	}

		//SSC_Disable(); // set SD_CS high
		// here is the right place to inrease the SPI baud rate to maximum
		//SSC_Enable(); // set SD_CS high

		// read CID register
		result = SDC_GetCID((uint8_t *)&SDCardInfo.CID);
		if(result != SD_SUCCESS)
		{
			printf("Error reading CID.\r\n");
			goto end;
		}

		// read CSD register
		result = SDC_GetCSD((uint8_t *)&SDCardInfo.CSD);
		if(result != SD_SUCCESS)
		{
			printf("Error reading CSD.");
			goto end;
		}

		printf("ok\r\n");

		uint8_t c_size_mult, read_bl_len;
		uint32_t c_size;

		switch(SDCardInfo.CSD[0]>>6) // check CSD Version
		{
		case 0x00: // if CSD is V1.0 structure (2GB limit)

			/*
			memory capacity = BLOCKNR * BLOCK_LEN
			BLOCKNR = (C_SIZE+1) * MULT
			MULT = 2^(C_SIZE_MULT+2)
			BLOCK_LEN = 2^READ_BL_LEN

			C_SIZE      is 12 bits [73:62] in CSD register
			C_SIZE_MULT is  3 bits [49:47] in CSD register
			READ_BL_LEN is  4 bits [83:80] in CSD register
			*/

			read_bl_len = (SDCardInfo.CSD[5] & 0x0F); 				//CSD[05] -> [87:80]
			c_size = ((uint32_t)(SDCardInfo.CSD[6] & 0x03))<<10; 	//CSD[06] -> [79:72]
			c_size |= ((uint32_t)SDCardInfo.CSD[7])<<2;				//CSD[07] -> [71:64]
			c_size |= (uint32_t)(SDCardInfo.CSD[8]>>6);				//CSD[08] -> [63:56]
			c_size_mult = (SDCardInfo.CSD[9] & 0x03)<<1;  			//CSD[09] -> [55:48]
			c_size_mult |=(SDCardInfo.CSD[10] & 0x80)>>7;			//CSD[10] -> [47:40]
			SDCardInfo.Capacity = (uint32_t)(c_size+1)*(1L<<(c_size_mult+2))*(1L<<read_bl_len);
			break;

		case 0x01: // if CSD is V2.0 structure (HC SD-Card > 2GB)

			/*
			memory capacity = (C_SIZE+1) * 512K byte
			C_SIZE is 22 bits [69:48] in CSR register
			*/

			c_size = ((uint32_t)(SDCardInfo.CSD[7] & 0x3F))<<16;	//CSD[07] -> [71:64]
			c_size |= ((uint32_t)SDCardInfo.CSD[8])<<8;				//CSD[08] -> [63:56]
			c_size |= (uint32_t)SDCardInfo.CSD[9];  				//CSD[09] -> [55:48];
		 	SDCardInfo.Capacity = (c_size + 1)* 512L * 1024L;
			break;

		default: //unknown CSD Version
			SDCardInfo.Capacity = 0;
			break;
		}

		switch(SDCardInfo.Version)
		{
		 	case VER_1X:
				printf("\r\n  SD-CARD V1.x");
				break;
		   	case VER_20:
				printf("\r\n  SD-CARD V2.0 or later");
		 	default:
		 		break;
		}
		uint16_t mb_size = (uint16_t)(SDCardInfo.Capacity/(1024L*1024L));
		printf("\r\n  Capacity = %i MB", mb_size);

		SDC_PrintCID((uint8_t *)&SDCardInfo.CID);
		SDCardInfo.Valid = 1;
		// jump point for error	condition before
		end:
		SSC_Disable();
	}
	else
	{
		SSC_Deinit();
		SDCardInfo.Valid = 0;
		result = SD_ERROR_NOCARD;
		printf("No Card in Slot.");
	}
	return(result);
}


//________________________________________________________________________________________________________________________________________
// Function: 	SDC_Deinit(void);
//
// Description:	This function deinitialises the SDCard interface.
//
//
// Returnvalue: the function returns 0 if the initialisation was successfull otherwise the function returns an errorcode.
//________________________________________________________________________________________________________________________________________

SD_Result_t SDC_Deinit(void)
{
	printf("\r\n SDC deinit...");
	SSC_Deinit();

 	SDCardInfo.Valid = 0;
	SDCardInfo.Capacity = 0;
	SDCardInfo.Version = VER_UNKNOWN;

	printf("ok");
	return(SD_SUCCESS);
}

//________________________________________________________________________________________________________________________________________
// Function: 	SDC_PutSector(uint32_t addr, const uint8_t *Buffer)
//
// Description:	This function writes one sector of data to the SSC
//
//
// Returnvalue: SD_Result_t
//________________________________________________________________________________________________________________________________________

SD_Result_t SDC_PutSector(uint32_t addr, const uint8_t *Buffer)
{
	uint8_t rsp;
	uint16_t a, crc16;
	uint16_t timeout = 0;
	SD_Result_t result = SD_ERROR_UNKNOWN;

	addr = addr << 9; // convert sectoradress to byteadress
	rsp = SDC_SendCMDR1(CMD_WRITE_SINGLE_BLOCK, addr);
	if (rsp != R1_NO_ERR)
	{
		result = SD_ERROR_BAD_RESPONSE;
		goto end;
	}

	for (a=0;a<20;a++)					// at least one byte
	{
		SSC_GetChar();
	}
	crc16 = CRC16(Buffer, 512);         // calc checksum for data block
	SSC_PutChar(DATA_START_TOKEN);		// send data start of header to the SSC

	for (a=0;a<512;a++)					// transmit one sector (normaly 512bytes) of data to the sdcard.
	{
		SSC_PutChar(Buffer[a]);
	}
	// write two bytes of crc16 to the sdcard
	SSC_PutChar((uint8_t)(crc16>>8)); 		// write high byte first
	SSC_PutChar((uint8_t)(0x00FF&crc16)); 	// lowbyte last

	do							  		// wait for data response token
	{
	 	rsp = SSC_GetChar();
		if(timeout++ > 500)
		{
			result = SD_ERROR_TIMEOUT;
		 	goto end;
		}
	}while((rsp & 0x11) != 0x01 );
	// analyse data response token
	switch(rsp & DATA_RESPONSE_MASK)
	{
		case DATA_RESPONSE_OK:
			result = SD_SUCCESS;
			break;
		case DATA_RESPONSE_CRC_ERR:
			result = SD_ERROR_CRC_DATA;
			goto end;
			break;
		case DATA_RESPONSE_WRITE_ERR:
			result = SD_ERROR_WRITE_DATA;
			goto end;
			break;
		default:
			result = SD_ERROR_UNKNOWN;
			goto end;
			break;

	}
	// wait 2 seconds until the sdcard is busy.
	rsp = SDC_WaitForBusy(2000);
	if(rsp != 0xFF)
	{
		result =  SD_ERROR_TIMEOUT;
 		goto end;
	}

	// check card status
	rsp = SDC_SendCMDR1(CMD_SEND_STATUS, 0);
	// first byte of R2 response is like R1 response
	if(rsp != R1_NO_ERR)
	{
		result =  SD_ERROR_BAD_RESPONSE;
		SSC_GetChar(); // read out 2nd byte
	 	goto end;
	}
	// 2nd byte of r2 response
	rsp = SSC_GetChar();
	if(rsp != R2_NO_ERR)
	{
		result =  SD_ERROR_WRITE_DATA;
		SSC_GetChar();
	 	goto end;
	}
	end:
	if(result != SD_SUCCESS)
	{
	 	printf("Error %02X writing data to sd card (R=%02X).\r\n", result, rsp);
	}
	return(result);
}




//________________________________________________________________________________________________________________________________________
// Function: 	SDC_GetSector(uint32_t addr,uint8_t *Buffer);
//
// Description:	This function reads one sector of data from the SSC
//
//
// Returnvalue: SD_Result_t
//________________________________________________________________________________________________________________________________________

SD_Result_t SDC_GetSector(uint32_t addr,uint8_t *Buffer)
{
	addr = addr << 9; // convert sectoradress to byteadress
	return SDC_GetData(CMD_READ_SINGLE_BLOCK, addr, Buffer, 512);
}

