#include <inttypes.h>
#include "ubx.h"
#include "timer0.h"
#include "uart0.h"
#include "printf_P.h"
#include "uart1.h"

// ------------------------------------------------------------------------------------------------
// defines

#define DAYS_FROM_JAN01YEAR0001_TO_JAN6_1980 722819 // the year 0 does not exist!
#define DAYS_PER_YEAR 		365
#define DAYS_PER_LEAPYEAR	366
#define DAYS_PER_4YEARS		1461 	//((3 * DAYS_PER_YEAR) + DAYS_PER_LEAPYEAR) // years dividable by 4 are leap years
#define DAYS_PER_100YEARS	36524 	//((25 * DAYS_PER_4YEARS) - 1) // years dividable by 100 are no leap years
#define DAYS_PER_400YEARS	146097	//((4 * DAYS_PER_100YEARS) + 1L) // but years dividable by 400 are leap years
#define SECONDS_PER_MINUTE	60
#define MINUTES_PER_HOUR	60
#define	HOURS_PER_DAY		24
#define DAYS_PER_WEEK		7
#define SECONDS_PER_HOUR	3600	//(SECONDS_PER_MINUTE * MINUTES_PER_HOUR)
#define SECONDS_PER_DAY		86400	//(SECONDS_PER_HOUR * HOURS_PER_DAY)
#define SECONDS_PER_WEEK	604800  //(SECONDS_PER_DAY * DAYS_PER_WEEK)

// days per month in normal and leap years
const uint32_t 	 Leap[ 13 ] = { 0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };
const uint32_t Normal[ 13 ]	= { 0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

#define LEAP_SECONDS_FROM_1980	15 // the last one was on the Dec 31th 2008

// message sync bytes
#define	UBX_SYNC1_CHAR	0xB5
#define	UBX_SYNC2_CHAR	0x62
// protocoll identifier
#define	UBX_CLASS_NAV	0x01
// message id
#define	UBX_ID_POSLLH	0x02
#define UBX_ID_SOL		0x06
#define	UBX_ID_VELNED	0x12

// ------------------------------------------------------------------------------------------------
// typedefs


// ubx parser state
typedef enum
{
	UBXSTATE_IDLE,
	UBXSTATE_SYNC1,
	UBXSTATE_SYNC2,
	UBXSTATE_CLASS,
	UBXSTATE_LEN1,
	UBXSTATE_LEN2,
	UBXSTATE_DATA,
	UBXSTATE_CKA,
	UBXSTATE_CKB
} ubxState_t;

typedef struct
{
	uint32_t	itow;		// ms GPS Millisecond Time of Week
	int32_t		frac;		// ns remainder of rounded ms above
	int16_t		week;		// GPS week
	uint8_t		GPSfix;		// GPSfix Type, range 0..6
	uint8_t		Flags;		// Navigation Status Flags
	int32_t		ECEF_X;		// cm ECEF X coordinate
	int32_t		ECEF_Y;		// cm ECEF Y coordinate
	int32_t		ECEF_Z;		// cm ECEF Z coordinate
	int32_t		PAcc;		// cm 3D Position Accuracy Estimate
	int32_t		ECEFVX;		// cm/s ECEF X velocity
	int32_t		ECEFVY;		// cm/s ECEF Y velocity
	int32_t		ECEFVZ;		// cm/s ECEF Z velocity
	uint32_t	SAcc;		// cm/s Speed Accuracy Estimate
	uint16_t	PDOP;		// 0.01 Position DOP
	uint8_t		res1;		// reserved
	uint8_t		numSV;		// Number of SVs used in navigation solution
	uint32_t	res2;		// reserved
	uint8_t		Status;	    // invalid/newdata/processed
} __attribute__((packed)) ubx_nav_sol_t;


typedef struct
{
	uint32_t	itow;  		// ms  GPS Millisecond Time of Week
	int32_t		VEL_N; 		// cm/s  NED north velocity
	int32_t		VEL_E; 		// cm/s  NED east velocity
	int32_t		VEL_D; 		// cm/s  NED down velocity
	int32_t		Speed; 		// cm/s  Speed (3-D)
	int32_t		GSpeed; 	// cm/s  Ground Speed (2-D)
	int32_t		Heading; 	// 1e-05 deg  Heading 2-D
	uint32_t	SAcc;		// cm/s  Speed Accuracy Estimate
	uint32_t	CAcc; 		// deg  Course / Heading Accuracy Estimate
	uint8_t		Status;		// invalid/newdata/processed
} __attribute__((packed)) ubx_nav_velned_t;

typedef struct
{
	uint32_t	itow;		// ms GPS Millisecond Time of Week
	int32_t		LON;		// 1e-07 deg Longitude
	int32_t		LAT;		// 1e-07 deg Latitude
	int32_t		HEIGHT;		// mm Height above Ellipsoid
	int32_t		HMSL;		// mm Height above mean sea level
	uint32_t	Hacc;		// mm Horizontal Accuracy Estimate
	uint32_t	Vacc;		// mm Vertical Accuracy Estimate
	uint8_t		Status;		// invalid/newdata/processed
} __attribute__((packed)) ubx_nav_posllh_t;



//------------------------------------------------------------------------------------
// global variables

// local buffers for the incomming ubx messages
volatile ubx_nav_sol_t		UbxSol	  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, INVALID};
volatile ubx_nav_posllh_t	UbxPosLlh = {0,0,0,0,0,0,0, INVALID};
volatile ubx_nav_velned_t	UbxVelNed = {0,0,0,0,0,0,0,0,0, INVALID};

uint16_t CheckGPSOkay = 0;

// shared buffer
gps_data_t  		GPSData = {{0,0,0,INVALID},0,0,0,0,0,0,0, INVALID};

//------------------------------------------------------------------------------------
// functions

uint8_t IsLeapYear(uint16_t year)
{
	if((year%400 == 0) || ( (year%4 == 0) && (year%100 != 0) ) ) return 1;
	else return 0;
}

/********************************************************/
/*  Calculates the UTC Time from the GPS week and tow   */
/********************************************************/
void SetGPSTime(DateTime_t * pTimeStruct)
{
	uint32_t Days, Seconds, Week;
	uint16_t YearPart;
	uint32_t * MonthDayTab = 0;
	uint8_t  i;



	// if GPS data show valid time data
	if((UbxSol.Status != INVALID) && (UbxSol.Flags & FLAG_WKNSET) && (UbxSol.Flags & FLAG_TOWSET) )
	{
		Seconds = UbxSol.itow / 1000L;
		Week = (uint32_t)UbxSol.week;
		// correct leap seconds since 1980
		if(Seconds < LEAP_SECONDS_FROM_1980)
		{
			Week--;
		 	Seconds = SECONDS_PER_WEEK - LEAP_SECONDS_FROM_1980 + Seconds;
		}
		else Seconds -= LEAP_SECONDS_FROM_1980;

	 	Days = DAYS_FROM_JAN01YEAR0001_TO_JAN6_1980;
		Days += (Week * DAYS_PER_WEEK);
		Days += Seconds / SECONDS_PER_DAY; // seperate days from GPS seconds of week

		pTimeStruct->Year = 1;
		YearPart = (uint16_t)(Days / DAYS_PER_400YEARS);
		pTimeStruct->Year += YearPart * 400;
		Days = Days % DAYS_PER_400YEARS;
		YearPart = (uint16_t)(Days / DAYS_PER_100YEARS);
		pTimeStruct->Year += YearPart * 100;
		Days = Days % DAYS_PER_100YEARS;
		YearPart = (uint16_t)(Days / DAYS_PER_4YEARS);
		pTimeStruct->Year += YearPart * 4;
		Days = Days % DAYS_PER_4YEARS;
		if(Days < (3* DAYS_PER_YEAR)) YearPart = (uint16_t)(Days / DAYS_PER_YEAR);
		else YearPart = 3;
		pTimeStruct->Year += YearPart;
		// calculate remaining days of year
		Days -= (uint32_t)(YearPart *  DAYS_PER_YEAR);
		Days += 1;
		// check if current year is a leap year
		if(IsLeapYear(pTimeStruct->Year)) MonthDayTab = (uint32_t*)Leap;
		else MonthDayTab = (uint32_t*)Normal;
	    // seperate month and day from days of year
		for ( i = 0; i < 12; i++ )
		{
			if ( (MonthDayTab[i]< Days) && (Days <= MonthDayTab[i+1]) )
			{
				pTimeStruct->Month = i+1;
				pTimeStruct->Day = Days - MonthDayTab[i];
				i = 12;
			}
		}
		Seconds = Seconds % SECONDS_PER_DAY; // remaining seconds of current day
		pTimeStruct->Hour = (uint8_t)(Seconds / SECONDS_PER_HOUR);
		Seconds = Seconds % SECONDS_PER_HOUR; // remaining seconds of current hour
		pTimeStruct->Min = (uint8_t)(Seconds / SECONDS_PER_MINUTE);
		Seconds = Seconds % SECONDS_PER_MINUTE; // remaining seconds of current minute
		pTimeStruct->Sec = (uint8_t)(Seconds);
		pTimeStruct->mSec  = (uint16_t)(UbxSol.itow % 1000L);
		pTimeStruct->Valid = 1;
	}
	else
	{
		pTimeStruct->Valid = 0;
	}
}



/********************************************************/
/*                  Initialize UBX Parser               */
/********************************************************/
void UBX_Init(void)
{
	printf("\r\n UBX init...");
	// mark msg buffers invalid
	UbxSol.Status = INVALID;
	UbxPosLlh.Status = INVALID;
	UbxVelNed.Status = INVALID;
	GPSData.Status = INVALID;
	printf("ok");
}

/********************************************************/
/*            Upate GPS data stcructure                 */
/********************************************************/
static uint16_t Ubx_Timeout = 0;void Update_GPSData (void)
{
	static uint8_t  Msg_Count = 0;

	// the timeout is used to detect the delay between two message sets
	// and is used for synchronisation so that always a set is collected
	// that belongs together
	// _______NAVSOL|POSLLH|VELNED|___________________NAVSOL|POSLLH|VELNED|_____________
	//              |  8ms | 8ms  |         184 ms          |      |      |
	// msg_count:   0      1      2                         0      1      2

	if(CheckDelay(Ubx_Timeout))	Msg_Count = 0;
	else Msg_Count++;
	Ubx_Timeout = SetDelay(100); // reset ubx msg timeout

	// if a new set of ubx messages was collected
	if((Msg_Count >= 2))
	{	// if set is complete
		if((UbxSol.Status == NEWDATA) && (UbxPosLlh.Status == NEWDATA) && (UbxVelNed.Status == NEWDATA))
		{//UDR0='0';
			CheckGPSOkay++;
			// update GPS data only if the status is INVALID or PROCESSED  and the last ubx message was received within less than 100 ms
			if(GPSData.Status != NEWDATA) // if last data were processed
			{ // wait for new data at all neccesary ubx messages
				GPSData.Status = INVALID;
				// NAV SOL
				GPSData.Flags =					UbxSol.Flags;
				GPSData.NumOfSats = 			UbxSol.numSV;
				GPSData.SatFix = 				UbxSol.GPSfix;
				GPSData.Position_Accuracy =		UbxSol.PAcc;
				GPSData.Speed_Accuracy = 		UbxSol.SAcc;
				SetGPSTime(&SystemTime); // update system time
				// NAV POSLLH
				GPSData.Position.Status = 		INVALID;
				
				//printf("%d\n",((char *)&UbxPosLlh.LON)[0]);
				GPSData.Position.Longitude =  	UbxPosLlh.LON;
				GPSData.Position.Latitude =  	UbxPosLlh.LAT;
				GPSData.Position.Altitude =  	UbxPosLlh.HMSL;
				GPSData.Position.Status = 		NEWDATA;
				// NAV VELNED
				GPSData.Speed_East = 			UbxVelNed.VEL_E;
				GPSData.Speed_North = 			UbxVelNed.VEL_N;
				GPSData.Speed_Top 	= 			-UbxVelNed.VEL_D;
				GPSData.Speed_Ground = 			UbxVelNed.GSpeed;
				GPSData.Heading = 				UbxVelNed.Heading;

				GPSData.Status = NEWDATA; // new data available
			} // EOF if(GPSData.Status != NEWDATA)
		} // EOF all ubx messages received
		// set state to collect new data
		UbxSol.Status = 				PROCESSED;	// ready for new data
		UbxPosLlh.Status = 				PROCESSED;	// ready for new data
		UbxVelNed.Status = 				PROCESSED;	// ready for new data
	}
}


/********************************************************/
/*                   UBX Parser                         */
/********************************************************/
void UBX_Parser()
{static int i=0;
	static ubxState_t ubxState = UBXSTATE_IDLE;
	static uint16_t msglen;
	static uint8_t cka, ckb;
	static uint8_t *ubxP, *ubxEp, *ubxSp; // pointers to data currently transfered
	unsigned char c;

	while(fifo_get(&rxFifo, &c)){//printf("%d %d\n",rxFifo.pread,rxFifo.pwrite);
	//state machine
	switch (ubxState)	// ubx message parser
	{
		case UBXSTATE_IDLE: // check 1st sync byte
			if (c == UBX_SYNC1_CHAR) ubxState = UBXSTATE_SYNC1;
			else ubxState = UBXSTATE_IDLE; // out of synchronization
			break;

		case UBXSTATE_SYNC1: // check 2nd sync byte
			if (c == UBX_SYNC2_CHAR) ubxState = UBXSTATE_SYNC2;
			else ubxState = UBXSTATE_IDLE; // out of synchronization
			break;

		case UBXSTATE_SYNC2: // check msg class to be NAV
			if (c == UBX_CLASS_NAV) ubxState = UBXSTATE_CLASS;
			else ubxState = UBXSTATE_IDLE; // unsupported message class
			break;

		case UBXSTATE_CLASS: // check message identifier
						
			switch(c)
			{
				case UBX_ID_POSLLH: // geodetic position
					ubxP =  (uint8_t *)&UbxPosLlh; // data start pointer
					ubxEp = (uint8_t *)(&UbxPosLlh + 1); // data end pointer
					ubxSp = (uint8_t *)&UbxPosLlh.Status; // status pointer
					break;

				case UBX_ID_SOL: // navigation solution
					ubxP =  (uint8_t *)&UbxSol; // data start pointer
					ubxEp = (uint8_t *)(&UbxSol + 1); // data end pointer
					ubxSp = (uint8_t *)&UbxSol.Status; // status pointer
					break;

				case UBX_ID_VELNED: // velocity vector in tangent plane
					ubxP =  (uint8_t *)&UbxVelNed; // data start pointer
					ubxEp = (uint8_t *)(&UbxVelNed + 1); // data end pointer
					ubxSp = (uint8_t *)&UbxVelNed.Status; // status pointer
					break;

				default://printf("%d\n",UbxPosLlh.LON);			// unsupported identifier
					ubxState = UBXSTATE_IDLE;
					break;
			}
			if (ubxState != UBXSTATE_IDLE)
			{
				ubxState = UBXSTATE_LEN1;
				cka = UBX_CLASS_NAV + c;
				ckb = UBX_CLASS_NAV + cka;
			}
			break;

		case UBXSTATE_LEN1: // 1st message length byte
			msglen = (uint16_t)c; // lowbyte first
			cka += c;
			ckb += cka;
			ubxState = UBXSTATE_LEN2;
			break;

		case UBXSTATE_LEN2: // 2nd message length byte
		
			msglen += ((uint16_t)c)<<8; // high byte last
			cka += c;
			ckb += cka;
			// if the old data are not processed so far then break parsing now
			// to avoid writing new data in ISR during reading by another function

				*ubxSp = INVALID; // mark invalid during buffer filling
				ubxState = UBXSTATE_DATA;

			break;

		case UBXSTATE_DATA: // collecting data
	//printf("%d %d\n",msglen,c);
			if (ubxP < ubxEp)
			{//if (ubxP ==  (uint8_t *)&UbxPosLlh)
				//printf("%d %d\n",msglen,c);
				*ubxP++ = c; // copy curent data byte if any space is left
				cka += c;
				ckb += cka;
				if (--msglen == 0) 	ubxState = UBXSTATE_CKA; // switch to next state if all data was read
			}
			else // rx buffer overrun
			{
				ubxState = UBXSTATE_IDLE;
			}
			break;

		case UBXSTATE_CKA:
			
			if (c == cka) ubxState = UBXSTATE_CKB;
			else
			{
				*ubxSp = INVALID;
				ubxState = UBXSTATE_IDLE;
			}
			break;

		case UBXSTATE_CKB:
			if (c == ckb)
			{
				*ubxSp = NEWDATA; // new data are valid
				Update_GPSData(); //update GPS info respectively
			}
			else
			{	// if checksum not match then set data invalid
				*ubxSp = INVALID;
			}
			ubxState = UBXSTATE_IDLE; // ready to parse new data
			break;

		default: // unknown ubx state
			ubxState = UBXSTATE_IDLE;
			break;

	}}
}
