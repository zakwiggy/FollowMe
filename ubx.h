#ifndef _UBX_H
#define _UBX_H

#include <inttypes.h>


// Satfix types for GPSData.SatFix
#define SATFIX_NONE				0x00
#define SATFIX_DEADRECKOING		0x01
#define SATFIX_2D				0x02
#define SATFIX_3D				0x03
#define SATFIX_GPS_DEADRECKOING	0x04
#define SATFIX_TIMEONLY			0x05
// Flags for interpretation of the GPSData.Flags
#define FLAG_GPSFIXOK			0x01 // (i.e. within DOP & ACC Masks)
#define FLAG_DIFFSOLN			0x02 // (is DGPS used)
#define FLAG_WKNSET				0x04 // (is Week Number valid)
#define FLAG_TOWSET				0x08 //	(is Time of Week valid)

#define	INVALID		0x00
#define NEWDATA		0x01
#define PROCESSED	0x02

typedef struct
{
	int32_t Longitude;  // in 1E-7 deg
	int32_t Latitude;	// in 1E-7 deg
	int32_t Altitude;	// in mm
	uint8_t Status;		// validity of data
} __attribute__((packed)) GPS_Pos_t;


typedef struct
{
	GPS_Pos_t	Position;       // Lat/Lon/Alt
	uint8_t		Flags;			// Status Flags
	uint8_t		NumOfSats;		// number of satelites
	uint8_t		SatFix;			// type of satfix
	uint32_t	Position_Accuracy;	// in cm 3d position accuracy
	int32_t		Speed_North;	// in cm/s
	int32_t		Speed_East;		// in cm/s
	int32_t		Speed_Top;		// in cm/s
	uint32_t	Speed_Ground;	// 2D ground speed in cm/s
	int32_t		Heading;		// 1e-05 deg  Heading 2-D (curent flight direction)
	uint32_t	Speed_Accuracy;	// in cm/s 3d velocity accuracy
	uint8_t		Status;			// status of data
} __attribute__((packed)) gps_data_t;

// The data are valid if the GPSData.Status is NEWDATA or PROCESSED.
// To achieve new data after reading the GPSData.Status should be set to PROCESSED.
extern gps_data_t  GPSData;
extern uint16_t CheckGPSOkay;

void UBX_Init(void);
void UBX_Parser(uint8_t c);

#endif // _UBX_H
