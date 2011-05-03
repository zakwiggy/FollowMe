#include "gps.h"
#include "uart0.h"
#include "main.h"
#include "timer0.h"

#define GPS_TIMEOUT 1000  // if no new gps data arrive within that time an error is set
#define GPS_MINSATS 4

//------------------------------------------------------------
// copy GPS position from source position to target position
uint8_t GPS_CopyPosition(GPS_Pos_t * pGPSPosSrc, GPS_Pos_t* pGPSPosTgt)
{
	uint8_t retval = 0;
	if((pGPSPosSrc == 0) || (pGPSPosTgt == 0)) return(retval);	// bad pointer
	// copy only valid positions
	if(pGPSPosSrc->Status != INVALID)
	{
		// if the source GPS position is not invalid
		pGPSPosTgt->Longitude	= pGPSPosSrc->Longitude;
		pGPSPosTgt->Latitude	= pGPSPosSrc->Latitude;
		pGPSPosTgt->Altitude	= pGPSPosSrc->Altitude;
		pGPSPosTgt->Status 		= NEWDATA; // mark data in target position as new
		retval = 1;
	}
	return(retval);
}

//------------------------------------------------------------
// clear position data
uint8_t GPS_ClearPosition(GPS_Pos_t * pGPSPos)
{
 	uint8_t retval = 0;
	if(pGPSPos == 0) return(retval);	// bad pointer
	else
	{
		pGPSPos->Longitude	= 0;
		pGPSPos->Latitude	= 0;
		pGPSPos->Altitude	= 0;
		pGPSPos->Status 	= INVALID;
		retval = 1;
	}
	return (retval);
}


//------------------------------------------------------------
// check for new GPS data
void GPS_Update(void)
{
	static uint16_t GPS_Timeout = 0;
	static uint16_t beep_rythm = 0;

	switch(GPSData.Status)
	{
		case INVALID:
			Error |= ERROR_GPS_RX_TIMEOUT;
			GPS_ClearPosition(&(FollowMe.Position)); // clear followme position
			break;

		case PROCESSED:
			// wait for timeout
			if(CheckDelay(GPS_Timeout))
			{
				GPSData.Status = INVALID;
			}
			break;

		case NEWDATA:
			GPS_Timeout = SetDelay(GPS_TIMEOUT); // reset gps timeout
			Error &= ~ERROR_GPS_RX_TIMEOUT; 	// clear possible error
			beep_rythm++;

			// update data in the follow me message
			if((GPSData.SatFix & SATFIX_3D) && (GPSData.NumOfSats >= GPS_MINSATS))
			{
					GPS_CopyPosition(&(GPSData.Position),&(FollowMe.Position));
			}
			else
			{
				GPS_ClearPosition(&(FollowMe.Position)); // clear followme position
		  	}

			// NC like sound on bad gps signals
			if(SysState == STATE_SEND_FOLLOWME)
			{
				if(!(GPSData.Flags & FLAG_GPSFIXOK) && !(beep_rythm % 5)) BeepTime = 100;
				else if ((GPSData.NumOfSats < GPS_MINSATS) && !(beep_rythm % 5)) BeepTime = 10;
			}

			GPSData.Status = PROCESSED; // set to processed unlocks the buffer for new data
			break;

		default:
			GPSData.Status = INVALID;
			break;
	}
}

