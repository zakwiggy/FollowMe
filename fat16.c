#include <string.h>
#include "printf_P.h"
#include "timer0.h"
#include "fat16.h"
#include "sdc.h"
#include "uart1.h"


/*
FAT16 Drive Layout:
Description 						Offset
Volume Boot Sector 					Start of Partition
Fat Tables							Start + # of Reserved Sectors
Root Directory Entry				Start + # of Reserved + (# of Sectors Per FAT * 2)
Data Area (Starts with Cluster #2)	Start + # of Reserved + (# of Sectors Per FAT * 2) + ((Maximum Root Directory Entries * 32) / Bytes per Sector)
*/


/*
________________________________________________________________________________________________________________________________________

	Structure of an partition entry
________________________________________________________________________________________________________________________________________

	Partition Entry is 16 bytes long
*/
typedef struct
{
	uint8_t		PartitionState;				// Current State of Partition (00h=Inactive, 80h=Active)
	uint8_t		BeginningHead;				// Beginning of Partition - Head
	uint16_t	BeginningCylSec;			// Beginning of Partition - Cylinder/Sector (See Below)
	uint8_t		Type;						// Type of Partition (See List Below)
	uint8_t		EndHead;					// End of Partition - Head
	uint16_t	EndCylSec;					// End of Partition - Cylinder/Sector
	uint32_t	NoSectorsBeforePartition;	// Number of Sectors between the MBR and the First Sector in the Partition
	uint32_t	NoSectorsPartition	;		// Number of Sectors in the Partition
} __attribute__((packed)) PartitionEntry_t;

/*
Coding of Cylinder/Sector words

Cylinder is 10 bits:  [7:0] at [15:8] and [9:8] at [7:6]
Sector is 5 bits:  [5:0] at [5:0]
*/

// Partition Types:
#define PART_TYPE_UNKNOWN			0x00
#define PART_TYPE_FAT12				0x01
#define PART_TYPE_XENIX				0x02
#define PART_TYPE_FAT16_ST_32_MB	0x04
#define PART_TYPE_EXTDOS			0x05
#define PART_TYPE_FAT16_LT_32_MB	0x06
#define PART_TYPE_NTFS				0x07
#define PART_TYPE_FAT32				0x0B
#define PART_TYPE_FAT32LBA			0x0C
#define PART_TYPE_FAT16LBA			0x0E
#define PART_TYPE_EXTDOSLBA			0x0F
#define PART_TYPE_EISA				0x12
#define PART_TYPE_ONTRACK			0x33
#define PART_TYPE_NOVELL			0x40
#define PART_TYPE_DYNAMIC			0x42
#define PART_TYPE_PCIX				0x4B
#define PART_TYPE_LINUX_SWAP		0x82
#define PART_TYPE_LINUX_NATIVE		0x83
#define PART_TYPE_LINUX_LVM			0x8E
#define PART_TYPE_PHOENIXSAVE		0xA0
#define PART_TYPE_FREEBSD			0xA5
#define PART_TYPE_OPENBSD			0xA6
#define PART_TYPE_NETNBSD			0xA9
#define PART_TYPE_CPM				0xDB
#define PART_TYPE_DBFS				0xE0
#define PART_TYPE_BBT				0xFF


/*
________________________________________________________________________________________________________________________________________

	Structure of the MasterBootRecord
________________________________________________________________________________________________________________________________________

	Master Boot Record is 512 bytes long
	The Master Boot Record is the same for pretty much all Operating Systems.
	It is located on the first Sector of the Hard Drive, at Cylinder 0, Head 0, Sector 1
*/
typedef struct
{
	uint8_t				ExecutableCode[446];	// 446 bytes for machine start code
	PartitionEntry_t	PartitionEntry1;		// 16 bytes for partition entry 1
	PartitionEntry_t	PartitionEntry2;		// 16 bytes for partition entry 2
	PartitionEntry_t	PartitionEntry3;		// 16 bytes for partition entry 3
	PartitionEntry_t	PartitionEntry4;		// 16 bytes for partition entry 4
	uint16_t			ExecutableMarker; 		// BIOS-Signature (0x55 0xAA)
} __attribute__((packed)) MBR_Entry_t;


/*
________________________________________________________________________________________________________________________________________

	Structure of the VolumeBootRecord
________________________________________________________________________________________________________________________________________

	The Volume Boot Record is 512 bytes long
	This information is located in the first sector of every partition.
*/
typedef struct
{
	uint8_t		JumpCode[3];  			// Jump Code + NOP
	int8_t		OEMName[8];				// OEM Name
	uint16_t	BytesPerSector;			// Bytes Per Sector
	uint8_t		SectorsPerCluster;		// Sectors Per Cluster
	uint16_t	ReservedSectors;		// Reserved Sectors
	uint8_t		NoFATCopies;			// Number of Copies of FAT
	uint16_t	MaxRootEntries;			// Maximum Root Directory Entries
	uint16_t	NoSectorsInPartSml32MB;	// Number of Sectors in Partition Smaller than 32 MB
	uint8_t		MediaDescriptor;		// Media Descriptor (0xF8 for Hard Disks)
	uint16_t	SectorsPerFAT;			// Sectors Per FAT
	uint16_t	SectorsPerTrack;		// Sectors Per Track
	uint16_t	NoHeads;				// Number of Heads
	uint32_t	NoHiddenSectors;		// Number of Hidden Sectors	in Partition
	uint32_t	NoSectors;				// Number of Sectors in Partition
	uint16_t	DriveNo;				// Logical Drive Number of Partition
	uint8_t		ExtendedSig;			// Extended Signature (0x29)
	uint32_t	SerialNo;				// Serial Number of the Partition
	int8_t		VolumeName[11];			// Volume Name of the Partititon
	int8_t		FATName[8];				// FAT Name (FAT16)
	uint8_t		ExecutableCode[446];	// 446 bytes for machine start code
	uint16_t	ExecutableMarker;		// Executable Marker (0x55 0xAA)
} __attribute__((packed)) VBR_Entry_t;



/*
________________________________________________________________________________________________________________________________________

	Structure of an directory entry
________________________________________________________________________________________________________________________________________

	Directory entry is 32 bytes.
*/
typedef struct
{
	int8_t		Name[8];					// 8 bytes name, padded with spaces.
	uint8_t		Extension[3];				// 3 bytes extension, padded with spaces.
	uint8_t		Attribute;					// attribute of the directory entry (unused,archive,read-only,system,directory,volume)
	uint8_t		Reserved[10];				// reserved bytes within the directory entry.
	uint32_t	DateTime;					// date and time of last write access to the file or directory.
	uint16_t	StartCluster;				// first cluster of the file or directory.
	uint32_t	Size;						// size of the file or directory in bytes.
}  __attribute__((packed)) DirEntry_t;

#define SLOT_EMPTY      	0x00	// slot has never been used
#define SLOT_E5         	0x05	// the real value is 0xe5
#define SLOT_DELETED    	0xE5	// file in this slot deleted

#define ATTR_NONE     		0x00	// normal file
#define ATTR_READONLY		0x01	// file is readonly
#define ATTR_HIDDEN			0x02	// file is hidden
#define ATTR_SYSTEM			0x04	// file is a system file
#define ATTR_VOLUMELABEL	0x08	// entry is a volume label
#define ATTR_LONG_FILENAME	0x0F	// this is a long filename entry
#define ATTR_SUBDIRECTORY	0x10	// entry is a directory name
#define ATTR_ARCHIVE		0x20	// file is new or modified


/*
________________________________________________________________________________________________________________________________________

	Structure of an entry within the fileallocationtable.
________________________________________________________________________________________________________________________________________
*/
typedef struct
{
	uint16_t  NextCluster;				// the next cluster of the file.
} __attribute__((packed)) Fat16Entry_t;

// secial fat entries
#define FAT16_CLUSTER_FREE 			0x0000
#define FAT16_CLUSTER_RESERVED		0x0001
#define FAT16_CLUSTER_USED_MIN		0x0002
#define FAT16_CLUSTER_USED_MAX		0xFFEF
#define FAT16_CLUSTER_ROOTDIR_MIN 	0xFFF0
#define FAT16_CLUSTER_ROOTDIR_MAX 	0xFFF6
#define FAT16_CLUSTER_BAD 			0xFFF7
#define FAT16_CLUSTER_LAST_MIN 		0xFFF8
#define FAT16_CLUSTER_LAST_MAX 		0xFFFF

/*****************************************************************************************************************************************/
/*																																		 */
/*	Global variables needed for read- or write-acces to the FAT16- filesystem.															 */
/*																																		 */
/*****************************************************************************************************************************************/

#define	MBR_SECTOR					0x00	// the masterboot record is located in sector 0.
#define DIRENTRY_SIZE				32		//bytes
#define DIRENTRIES_PER_SECTOR		BYTES_PER_SECTOR/DIRENTRY_SIZE
#define FAT16_BYTES					2
#define FAT16_ENTRIES_PER_SECTOR	BYTES_PER_SECTOR/FAT16_BYTES

#define	FSTATE_UNUSED	0
#define	FSTATE_USED		1

typedef struct
{
	uint8_t		IsValid;				// 0 means invalid, else valid
	uint8_t		SectorsPerCluster;		// how many sectors does a cluster contain?
	uint8_t		FatCopies;				// Numbers of copies of the FAT
	uint16_t	MaxRootEntries;			// Possible number of entries in the root directory.
	uint16_t	SectorsPerFat;			// how many sectors does a fat16 contain?
	uint32_t	FirstFatSector;			// sector of the start of the fat
	uint32_t	FirstRootDirSector;		// sector of the rootdirectory
	uint32_t	FirstDataSector;		// sector of the first cluster containing data (cluster2).
	uint32_t	LastDataSector;			// the last data sector of the partition
} Partition_t;

Partition_t 	Partition;					// Structure holds partition information

File_t FilePointer[FILE_MAX_OPEN];	// Allocate Memmoryspace for each filepointer used.


/****************************************************************************************************************************************/
/*	Function: 		FileDateTime(DateTime_t *);																							*/
/*																																	  	*/
/*	Description:	This function calculates the DOS date time from a pointer to a time structure.										*/
/*																																	   	*/
/*	Returnvalue:	Returns the DOS date time.																							*/
/****************************************************************************************************************************************/

uint32_t FileDateTime(DateTime_t * pTimeStruct)
{
	uint32_t datetime = 0;
	if((pTimeStruct == 0) || !(pTimeStruct->Valid)) return datetime;

	datetime |= (0x0000007FL & (uint32_t)(pTimeStruct->Year - 1980))<<25; // set year
	datetime |= (0x0000000FL & (uint32_t)(pTimeStruct->Month))<<21; // set month
	datetime |= (0x0000001FL & (uint32_t)(pTimeStruct->Day))<<16;
	datetime |= (0x0000001FL & (uint32_t)(pTimeStruct->Hour))<<11;
	datetime |= (0x0000003FL & (uint32_t)(pTimeStruct->Min))<<5;
	datetime |= (0x0000001FL & (uint32_t)(pTimeStruct->Sec/2));
	return datetime;
}


/****************************************************************************************************************************************/
/*	Function: 		LockFilePointer();																									*/
/*																																	  	*/
/*	Description:	This function trys to lock a free file pointer.																		*/
/*																																	   	*/
/*	Returnvalue:	Returns the Filepointer on success or 0.																			*/
/****************************************************************************************************************************************/
File_t * LockFilePointer(void)
{
	uint8_t i;
	File_t * File = 0;
	for(i = 0; i < FILE_MAX_OPEN; i++)
	{
		if(FilePointer[i].State == FSTATE_UNUSED)		// found an unused one
		{
			File = &FilePointer[i];						// set pointer to that entry
			FilePointer[i].State = FSTATE_USED;			// mark it as used
			break;
		}
	}
	return(File);
}

/****************************************************************************************************************************************/
/*	Function: 		UnlockFilePointer(file_t *);																						*/
/*																																	  	*/
/*	Description:	This function trys to unlock a file pointer.																		*/
/*																																	   	*/
/*	Returnvalue:	Returns 1 if file pointer was freed else 0.																			*/
/****************************************************************************************************************************************/
uint8_t UnlockFilePointer(File_t * file)
{
	uint8_t cnt;
	if(file == NULL) return(0);
	for(cnt = 0; cnt < FILE_MAX_OPEN; cnt++)
	{
		if(&FilePointer[cnt] == file)						// filepointer to be freed found?
		{
			file->State = FSTATE_UNUSED;
			file->FirstSectorOfFirstCluster	= 0;			// Sectorpointer to the first sector of the first datacluster of the file.
			file->FirstSectorOfCurrCluster 	= 0;
			file->SectorOfCurrCluster		= 0;			// Pointer to the cluster which is edited at the moment.
			file->SectorOfCurrCluster		= 0;			// The sector which is edited at the moment (cluster_pointer + sector_index).
			file->ByteOfCurrSector			= 0;			// The bytelocation within the current sector (cluster_pointer + sector_index + byte_index).
			file->Mode 						= 0;			// mode of fileoperation (read,write)
			file->Size 						= 0;			// the size of the opend file in bytes.
			file->Position 					= 0;			// pointer to a character within the file 0 < fileposition < filesize
			file->SectorInCache 			= 0;			// the last sector read, wich is still in the sectorbuffer.
			file->DirectorySector 			= 0;			// the sectorposition where the directoryentry has been made.
			file->DirectoryIndex 			= 0;			// the index to the directoryentry within the specified sector.
			file->Attribute 				= 0;			// the attribute of the file opened.
			file = NULL;
			return(1);
		}
	}
	return(0);
}

/****************************************************************************************************************************************/
/*	Function: 		SeperateDirName(int8_t*, int8_t*);																						*/
/*																																	  	*/
/*	Description:	This function seperates the first dirname from filepath and brings them												*/
/* 					into the needed format ('test.txt' -> 'TEST    TXT')																*/
/*					The subpath is the pointer to the remaining substring if the filepath 											   	*/
/*																																	   	*/
/*	Returnvalue: 	Return NULL on error or pointer to subpath 																									*/
/****************************************************************************************************************************************/
int8_t* SeperateDirName(const int8_t *filepath, int8_t *dirname)
{
	int8_t* subpath = NULL;
	uint8_t readpointer	= 0;
	uint8_t writepointer = 0;

	// search subpath from beginning of filepath
	subpath = NULL;
	readpointer	= 0;
	if(filepath[0] == '/') readpointer = 1; // ignore first '/'
	while(subpath == NULL)	// search the filepath until a subpath was found.
	{
		if(((filepath[readpointer] == 0) || (filepath[readpointer] == '/')))	// if '/' found or end of filepath reached
		{
			subpath = (int8_t*)&filepath[readpointer];				// store the position of the first "/" found after the beginning of the filenpath
		}
		readpointer++;
	}

	// clear dirname with spaces
	dirname[11] = 0; // terminate dirname
	for(writepointer = 0; writepointer < 11; writepointer++) dirname[writepointer] = ' ';
	writepointer = 0;
	// start seperating the dirname from the filepath.
	readpointer = 0;
	if(filepath[0] == '/') readpointer = 1; // ignore first '/'
	while( &filepath[readpointer] < subpath)
	{
		if(writepointer >= 11) return(NULL);		// dirname to long
		if(filepath[readpointer] == '.')			// seperating dirname and extension.
		{
			if(writepointer <= 8)
			{
				readpointer++;						// next character in filename
				writepointer = 8;					// jump to start of extension
			}
			else return(NULL);						// dirbasename to long
		}
		else
		{
			if((0x60 < filepath[readpointer]) && (filepath[readpointer] < 0x7B))
			{
				dirname[writepointer] = (filepath[readpointer] - 0x20);					// all characters must be upper case.
			}
			else
			{
				dirname[writepointer] = filepath[readpointer];
			}
			readpointer++;
			writepointer++;
		}
	}
	return(subpath);
}


/**************************************************************************************************************************************+*/
/*	Function: 	Fat16ClusterToSector( uint16_t cluster);																						*/
/*																																	  	*/
/*	Description:	This function converts a cluster number given by the fat to the corresponding										*/
/*					sector that points to the start of the data area that is represented by the cluster number.							*/
/*																																	   	*/
/*	Returnvalue: The sector number with the data area of the given cluster																*/
/****************************************************************************************************************************************/
uint32_t	Fat16ClusterToSector(uint16_t cluster)
{
	if(!Partition.IsValid) return 0;
	if (cluster < 2) cluster = 2; // the 0. and 1. cluster in the fat are used for the media descriptor
	return ( (cluster - 2) * Partition.SectorsPerCluster) + Partition.FirstDataSector; // the first data sector	is represented by the 2nd cluster
}

/****************************************************************************************************************************************/
/*	Function: 	SectorToFat16Cluster( uint32_t sector);																						*/
/*																																	  	*/
/*	Description:	This function converts a given sector number given to the corresponding												*/
/*					cluster number in the fat that represents this data area.															*/
/*																																	   	*/
/*	Returnvalue: The cluster number representing the data area of the sector.															*/
/****************************************************************************************************************************************/
uint16_t	SectorToFat16Cluster(uint32_t sector)
{
	if(!Partition.IsValid) return 0;
	return ((uint16_t)((sector - Partition.FirstDataSector) / Partition.SectorsPerCluster) + 2);
}


/****************************************************************************************************************************************/
/*	Function: 	Fat16_Deinit(void);																									   	*/
/*																																	   	*/
/*	Description:	This function uninitializes the fat 16 api																		   	*/
/*																																	   	*/
/*	Returnvalue: The function returns "0" on success														   							*/
/****************************************************************************************************************************************/
uint8_t Fat16_Deinit(void)
{
	int16_t returnvalue = 0;
	uint8_t cnt;
	// declare the filepointers as unused.
	for(cnt = 0; cnt < FILE_MAX_OPEN; cnt++)
	{
		if(FilePointer[cnt].State == FSTATE_USED)
		{
			returnvalue += fclose_(&FilePointer[cnt]); // try to close open file pointers
		}

	}
	SDC_Deinit();			// uninitialize interface to sd-card
	Partition.IsValid = 0;	// mark data in partition structure as invalid
	return(returnvalue);
}

/****************************************************************************************************************************************/
/*	Function: 		Fat16_Init(void);																									*/
/*																																	    */
/*	Description:	This function reads the Masterbootrecord and finds the position of the Volumebootrecord, the FAT and the Rootdir    */
/*					and stores the information in global variables.																	    */
/*																																	    */
/*	Returnvalue: 	The function returns "0" if the filesystem is initialized.															*/
/****************************************************************************************************************************************/
uint8_t Fat16_Init(void)
{
    uint8_t	cnt	= 0;
	uint32_t	partitionfirstsector;
	VBR_Entry_t *VBR;
	MBR_Entry_t *MBR;
	File_t *file;
	uint8_t result = 0;

	printf("\r\n FAT16 init...");
	Partition.IsValid = 0;

	// declare the filepointers as unused.
	for(cnt = 0; cnt < FILE_MAX_OPEN; cnt++)
	{
		FilePointer[cnt].State = FSTATE_UNUSED;
	}
	// set current file pinter to first position in list
	file = &FilePointer[0];

	// try to initialise the sd-card.
	if(SD_SUCCESS != SDC_Init())
	{
	 	printf("SD-Card could not be initialized.");
		result = 1;
		goto end;
	}

	// SD-Card is initialized successfully
	if(SD_SUCCESS != SDC_GetSector((uint32_t)MBR_SECTOR,file->Cache))	// Read the MasterBootRecord
	{
		printf("Error reading the MBR.");
		result = 2;
		goto end;
	}
	MBR = (MBR_Entry_t *)file->Cache;						// Enter the MBR using the structure MBR_Entry_t.
	if((MBR->PartitionEntry1.Type == PART_TYPE_FAT16_ST_32_MB) ||
	   (MBR->PartitionEntry1.Type == PART_TYPE_FAT16_LT_32_MB) ||
	   (MBR->PartitionEntry1.Type == PART_TYPE_FAT16LBA))
	{
		// get sector offset 1st partition
		partitionfirstsector = MBR->PartitionEntry1.NoSectorsBeforePartition;
		// Start of Partition is the Volume Boot Sector
		if(SD_SUCCESS != SDC_GetSector(partitionfirstsector,file->Cache)) // Read the volume boot record
		{
			printf("Error reading the VBR.");
			result = 3;
			goto end;
		}
	}
	else  // maybe the medium has no partition assuming sector 0 is the vbr
	{
	 	partitionfirstsector = 0;
	}

	VBR = (VBR_Entry_t *) file->Cache;						// Enter the VBR using the structure VBR_Entry_t.
	if(VBR->BytesPerSector != BYTES_PER_SECTOR)
	{
		printf("VBR: Sector size not supported.");
		result = 4;
		goto end;
	}
	Partition.SectorsPerCluster		= VBR->SectorsPerCluster;			// Number of sectors per cluster. Depends on the memorysize of the sd-card.
	Partition.FatCopies 			= VBR->NoFATCopies;					// Number of fatcopies.
	Partition.MaxRootEntries		= VBR->MaxRootEntries;				// How many Entries are possible in the rootdirectory (FAT16 allows max. 512 entries).
	Partition.SectorsPerFat 		= VBR->SectorsPerFAT;				// The number of sectors per FAT.

	/* Calculate the sectorpositon of the FAT, the Rootdirectory and the first Datacluster. */
	// Calculate the position of the FileAllocationTable:
	// Start + # of Reserved Sectors
	Partition.FirstFatSector	=   (uint32_t)(partitionfirstsector + (uint32_t)(VBR->ReservedSectors));
	// Calculate the position of the Rootdirectory:
	// Start + # of Reserved Sectors + (# of Sectors Per FAT * # of FAT Copies)
	Partition.FirstRootDirSector	=   Partition.FirstFatSector + (uint32_t)((uint32_t)Partition.SectorsPerFat*(uint32_t)Partition.FatCopies);
	// Calculate the position of the first datacluster:
	// Start + # of Reserved + (# of Sectors Per FAT * # of FAT Copies) + ((Maximum Root Directory Entries * 32) / Bytes per Sector)
	Partition.FirstDataSector	=   Partition.FirstRootDirSector + (uint32_t)(Partition.MaxRootEntries>>4);  // assuming 512 Byte Per Sector
	// Calculate the last data sector
	if(VBR->NoSectors == 0)
	{
	 	printf("VBR: Bad number of sectors.");
		result = 5;
		goto end;
	}
	Partition.LastDataSector = Partition.FirstDataSector + VBR->NoSectors - 1;
	// check for FAT16 in VBR of first partition
	if(!((VBR->FATName[0]=='F') && (VBR->FATName[1]=='A') && (VBR->FATName[2]=='T') && (VBR->FATName[3]=='1')&&(VBR->FATName[4]=='6')))
	{
		printf("VBR: Partition ist not FAT16 type.");
		result = 6;
		goto end;
	}
	Partition.IsValid = 1; // mark data in partition structure as valid
	result = 0;
	end:
	if(result != 0)	Fat16_Deinit();
	else printf(" ...ok");
	return(result);
}

/****************************************************************************************************************************************/
/*	Function: 	Fat16_IsValid(void);																									   	*/
/*																																	   	*/
/*	Description:	This function return the Fat 15 filesystem state																		   	*/
/*																																	   	*/
/*	Returnvalue: The function returns "1" on success														   							*/
/****************************************************************************************************************************************/
uint8_t Fat16_IsValid(void)
{
	return(Partition.IsValid);
}

/****************************************************************************************************************************************/
/* Function: 	ClearCurrCluster(File_t*);																							*/
/* 																																		*/
/* Description:	This function fills the current cluster with 0. 										 							 	*/
/*																																	 	*/
/* Returnvalue: The function returns 1 on success else 0.												 							 	*/
/****************************************************************************************************************************************/
uint8_t ClearCurrCluster(File_t * file)
{
	uint8_t retvalue = 1;
	uint32_t i;

	if((!Partition.IsValid) || (file == NULL)) return(0);

	for(i = 0; i < BYTES_PER_SECTOR; i++) file->Cache[i] = 0; // clear file cache
	for(i = 0; i < Partition.SectorsPerCluster; i++)
	{
		file->SectorInCache = file->FirstSectorOfCurrCluster + i;
	 	if(SD_SUCCESS != SDC_PutSector(file->SectorInCache, file->Cache))
		{
			Fat16_Deinit();
			retvalue = 0;
		}
	}
	return(retvalue);
}

/*****************************************************************************************************************************************/
/* Function: 	GetNextCluster(File_t* );																							 */
/* 																																		 */
/* Description:	This function finds the next datacluster of the file specified with File *File. 										 */
/*																																		 */
/* Returnvalue: The function returns the next cluster or 0 if the last cluster has already reached.													 */
/*****************************************************************************************************************************************/
uint16_t GetNextCluster(File_t * file)
{
	uint16_t cluster = 0;
	uint32_t fat_byte_offset, sector, byte;
	Fat16Entry_t * fat;

	if((!Partition.IsValid) || (file == NULL)) return(cluster);
	// if sector is within the data area
	if((Partition.FirstDataSector <= file->FirstSectorOfCurrCluster)&& (file->FirstSectorOfCurrCluster <= Partition.LastDataSector))
	{
		// determine current file cluster
		cluster = SectorToFat16Cluster(file->FirstSectorOfCurrCluster);
		// calculate byte offset in the fat for corresponding entry
		fat_byte_offset = ((uint32_t)cluster)<<1; // two FAT bytes (16 bits) for every cluster
		// calculate the sector that contains the current cluster within the fat
		sector = Partition.FirstFatSector + ( fat_byte_offset / BYTES_PER_SECTOR);
		// calculate byte offset of the current cluster within that fat sector
		byte = fat_byte_offset % BYTES_PER_SECTOR;
		// read this sector to the file cache
		if(file->SectorInCache != sector)
		{
			file->SectorInCache = sector;						// update sector stored in buffer
		 	if(SD_SUCCESS != SDC_GetSector(file->SectorInCache, file->Cache)) 	// read sector from sd-card
			{
				Fat16_Deinit();
				return (cluster);
			}
		}
		// read the next cluster from cache
		fat = (Fat16Entry_t *)(&(file->Cache[byte]));
		cluster = fat->NextCluster;
		// if last cluster fat entry
		if(FAT16_CLUSTER_LAST_MIN <= cluster)
		{
		 	 cluster = 0;
		}
		else
		{
			file->FirstSectorOfCurrCluster = Fat16ClusterToSector(cluster);
			file->SectorOfCurrCluster = 0;
			file->ByteOfCurrSector = 0;
		}
	}
	return(cluster);
}


/****************************************************************************************************************************************/
/* Function: 	FindNextFreeCluster(File_t *);																						*/
/* 																																		*/
/* Description:	This function looks in the fat to find the next free cluster 										 					*/
/*																																		*/
/* Returnvalue: The function returns the cluster number of the next free cluster found within the fat.									*/
/****************************************************************************************************************************************/
uint16_t FindNextFreeCluster(File_t *file)
{
	uint32_t fat_sector;				// current sector within the fat relative to the first sector of the fat.
	uint32_t curr_sector;				// current sector
	uint16_t fat_entry;					// index to an fatentry within the actual sector (256 fatentries are possible within one sector).
	uint16_t free_cluster = 0;			// next free cluster number.
	Fat16Entry_t * fat;

	if((!Partition.IsValid) || (file == NULL)) return(0);

	// start searching for an empty cluster at the beginning of the fat.
	fat_sector = 0;
	do
	{
		curr_sector = Partition.FirstFatSector + fat_sector;	// calculate sector to read
		file->SectorInCache = curr_sector;						// upate the sector number of file cache.
		if( SD_SUCCESS != SDC_GetSector(file->SectorInCache, file->Cache))		// read sector of fat from sd-card.
		{
			Fat16_Deinit();
			return(free_cluster);
		}

		fat = (Fat16Entry_t *)file->Cache;						// set fat pointer to file cache

		for(fat_entry = 0; fat_entry < FAT16_ENTRIES_PER_SECTOR; fat_entry++)						// look for an free cluster at all entries in this sector of the fat.
		{
			if(fat[fat_entry].NextCluster == FAT16_CLUSTER_FREE)		// empty cluster found!!
			{
				fat[fat_entry].NextCluster = FAT16_CLUSTER_LAST_MAX;	// mark this fat-entry as used
				if(SD_SUCCESS != SDC_PutSector(file->SectorInCache, file->Cache))		// and save the sector at the sd-card.
				{
					Fat16_Deinit();
					return(free_cluster);
				}
				free_cluster = (uint16_t)(fat_sector * FAT16_ENTRIES_PER_SECTOR + (uint32_t)fat_entry);
				fat_entry = FAT16_ENTRIES_PER_SECTOR;					// terminate the search for a free cluster in this sector.
			}
		}
		fat_sector++;													// continue the search in next fat sector
	// repeat until the end of the fat is  reached and no free cluster has been found so far
	}while((fat_sector < Partition.SectorsPerFat) && (!free_cluster));
	return(free_cluster);
}


/****************************************************************************************************************************************************/
/* Function: 	int16_t fseek_(File_t *, int32_t *, uint8_t)																							   			*/
/* 																																				   	*/
/* Description:	This function sets the pointer of the stream relative to the position																*/
/*				specified by origin (SEEK_SET, SEEK_CUR, SEEK_END)																					*/
/* Returnvalue: Is 1 if seek was successful																																	*/
/****************************************************************************************************************************************************/
int16_t fseek_(File_t * const file, int32_t offset, int16_t origin)
{
	int32_t		fposition 	= 0;
	int16_t 	retvalue 	= 1;

	if((!Partition.IsValid) || (file == NULL)) return(0);
	switch(origin)
	{
		case SEEK_SET:				// Fileposition relative to the beginning of the file.
			fposition = 0;
			break;
		case SEEK_END:				// Fileposition relative to the end of the file.
			fposition = (int32_t)file->Size;
			break;
		case SEEK_CUR:				// Fileposition relative to the current position of the file.
		default:
			fposition = file->Position;
			break;
	}

	fposition += offset;

	if((fposition >= 0) && (fposition <= (int32_t)file->Size))		// is the pointer still within the file?
	{
		// reset file position to start of the file
		file->FirstSectorOfCurrCluster = file->FirstSectorOfFirstCluster;
		file->SectorOfCurrCluster	= 0;
		file->ByteOfCurrSector		= 0;
		file->Position 				= 0;

		while(file->Position < fposition) 	// repeat until the current position is less than target
		{
			file->Position++;				// increment file position
			file->ByteOfCurrSector++;		// next byte in current sector
			if(file->ByteOfCurrSector >= BYTES_PER_SECTOR)
			{
				file->ByteOfCurrSector = 0;										// reading at the beginning of new sector.
				file->SectorOfCurrCluster++;									// continue reading in next sector
				if(file->SectorOfCurrCluster >= Partition.SectorsPerCluster)	// if end of cluster is reached, the next datacluster has to be searched in the FAT.
				{
					if(GetNextCluster(file))									// Sets the clusterpointer of the file to the next datacluster.
					{
						file->SectorOfCurrCluster = 0;
					}
					else // the last cluster was allready reached
					{
						file->SectorOfCurrCluster--;							// jump back to the ast sector in the last cluster
						file->ByteOfCurrSector = BYTES_PER_SECTOR;				// set ByteOfCurrSector one byte over sector end
					}
				}
			}
		}
	}
	if(file->Position == fposition) retvalue = 0;
	return(retvalue);
}


/****************************************************************************************************************************************/
/* Function: 	uint16_t DeleteClusterChain(File *file);																						*/
/* 																																		*/
/* Description:	This function trances along a cluster chain in the fat and frees all clusters visited.	 								*/
/*																																		*/
/****************************************************************************************************************************************/
uint8_t DeleteClusterChain(uint16_t StartCluster)
{
	uint16_t cluster;
	uint32_t fat_byte_offset, sector, byte;
	Fat16Entry_t * fat;
	uint8_t buffer[BYTES_PER_SECTOR];
	uint32_t sector_in_buffer = 0;
	uint8_t repeat = 0;

	if(!Partition.IsValid) return 0;

	cluster = StartCluster; // init chain trace
	// calculate byte offset in the fat for corresponding entry
	fat_byte_offset = ((uint32_t)cluster)<<1; // two FAT bytes (16 bits) for every cluster
	// calculate the sector that contains the current cluster within the fat
	sector = Partition.FirstFatSector + ( fat_byte_offset / BYTES_PER_SECTOR);
	// calculate byte offset of the current cluster within that fat sector
	byte = fat_byte_offset % BYTES_PER_SECTOR;
	do
	{
		if(sector != sector_in_buffer)
		{
			// read this sector to buffer
			sector_in_buffer = sector;
			if(SD_SUCCESS != SDC_GetSector(sector_in_buffer, buffer)) return 0; 	// read sector from sd-card
		}
		// read the next cluster from cache
		fat = (Fat16Entry_t *)(&(buffer[byte]));
		cluster = fat->NextCluster;
		if((FAT16_CLUSTER_USED_MIN <= cluster) && (cluster <= FAT16_CLUSTER_USED_MAX) ) repeat = 1;
		else repeat = 0;

		fat->NextCluster = 	FAT16_CLUSTER_FREE; // mark current cluster as free
		// calculate byte offset in the fat for corresponding entry
		fat_byte_offset = ((uint32_t)cluster)<<1; // two FAT bytes (16 bits) for every cluster
		// calculate the sector that contains the current cluster within the fat
		sector = Partition.FirstFatSector + ( fat_byte_offset / BYTES_PER_SECTOR);
		// calculate byte offset of the current cluster within that fat sector
		byte = fat_byte_offset % BYTES_PER_SECTOR;
		// if new sector is not the sector in buffer or the last cluster in the chain was traced
		if((sector != sector_in_buffer) || !repeat)
		{	// write sector in buffer
			if(SD_SUCCESS != SDC_PutSector(sector_in_buffer,buffer)) return 0;
		}
	}
	while(repeat);

	return 1;
}


/****************************************************************************************************************************************/
/* Function: 	uint16_t AppendCluster(File *file);																							*/
/* 																																		*/
/* Description:	This function looks in the fat to find the next free cluster and appends it to the file.	 							*/
/*																																		*/
/* Returnvalue: The function returns the appened cluster number or 0 of no cluster was appended.																		*/
/****************************************************************************************************************************************/
uint16_t AppendCluster(File_t *file)
{
	uint16_t last_cluster, new_cluster = 0;
	uint32_t fat_byte_offset, sector, byte;
	Fat16Entry_t * fat;

	if((!Partition.IsValid) || (file == NULL)) return(new_cluster);

	new_cluster = FindNextFreeCluster(file);	// the next free cluster found on the disk.
	if(new_cluster)
	{	// A free cluster was found and can be added to the end of the file.
		fseek_(file, 0, SEEK_END); 													// jump to the end of the file
		last_cluster = SectorToFat16Cluster(file->FirstSectorOfCurrCluster);		// determine current file cluster
		fat_byte_offset = ((uint32_t)last_cluster)<<1;
		sector = Partition.FirstFatSector + ( fat_byte_offset / BYTES_PER_SECTOR);
		byte = fat_byte_offset % BYTES_PER_SECTOR;

		if(file->SectorInCache != sector)
		{
			file->SectorInCache = sector;						// update sector stored in buffer
			if(SD_SUCCESS != SDC_GetSector(file->SectorInCache, file->Cache)) 	// read sector from sd-card
			{
				Fat16_Deinit();
			 	return(0);
			}
		}
		fat = (Fat16Entry_t *)(&(file->Cache[byte]));
		fat->NextCluster = new_cluster;							// append the free cluster to the end of the file in the FAT.
		if(SD_SUCCESS != SDC_PutSector(file->SectorInCache, file->Cache))		// save the modified sector to the FAT.
		{
			Fat16_Deinit();
		 	return(0);
		}
		file->FirstSectorOfCurrCluster = Fat16ClusterToSector(new_cluster);
		file->SectorOfCurrCluster = 0;
		file->ByteOfCurrSector = 0;
	}
	return(new_cluster);
}

/****************************************************************************************************************************************************/
/* Function: 	DirectoryEntryExist(int8_t *, uint8_t, uint8_t, File_t *)																							*/
/* 																																				   	*/
/* Description:	This function searches all possible dir entries until the file or directory is found or the end of the directory is reached			*/
/*																																					*/
/* Returnvalue: This function returns 1 if the directory entry specified was found.																	*/
/****************************************************************************************************************************************************/
uint8_t DirectoryEntryExist(int8_t *dirname, uint8_t attribfilter, uint8_t attribmask, File_t *file)
{
	uint32_t	dir_sector, max_dir_sector, curr_sector;
	uint16_t 	dir_entry = 0;

	uint16_t   	end_of_directory_not_reached = 0;
	uint8_t		i = 0;
	uint8_t  	direntry_exist = 0;
	DirEntry_t * dir;

	// if incomming pointers are useless return immediatly
	if((!Partition.IsValid) || (file == NULL) || (dirname == NULL)) return(direntry_exist);

	// dir entries can be searched only in filesclusters that have
	// a corresponding dir entry with adir-flag set in its attribute
	// or direct within the root directory area

	file->FirstSectorOfFirstCluster = 0;
	// no current directory exist therefore assume searching in the root
	if(file->DirectorySector == 0)
	{
		max_dir_sector = (Partition.MaxRootEntries * DIRENTRY_SIZE)/BYTES_PER_SECTOR;
		file->FirstSectorOfFirstCluster = Partition.FirstRootDirSector;
	}
   	// within the root directory area we can read sectors sequentially until the end of this area
	else if((Partition.FirstRootDirSector <= file->DirectorySector) && (file->DirectorySector < Partition.FirstDataSector))
	{
	   	max_dir_sector = (Partition.MaxRootEntries * DIRENTRY_SIZE)/BYTES_PER_SECTOR;
	}
	// within the data clusters we can read sectors sequentially only within the cluster
	else if((Partition.FirstDataSector <= file->DirectorySector) && (file->DirectorySector <= Partition.LastDataSector))
	{
		max_dir_sector = Partition.SectorsPerCluster;				// limit max secters before next cluster
	}
	else return (direntry_exist); // bad sector range for directory sector of the file
	// if search area is not defined yet
	if(file->FirstSectorOfFirstCluster == 0)
	{
		// check if the directory entry of current file is existent and has the dir-flag set
		file->SectorInCache = file->DirectorySector;				// update the sector number of file cache.
		if(SD_SUCCESS != SDC_GetSector(file->SectorInCache, file->Cache))// read in the sector.
		{
		 	Fat16_Deinit();
			return(direntry_exist);
		}
		dir = (DirEntry_t *)file->Cache;							// set pointer to directory
		switch((uint8_t)dir[file->DirectoryIndex].Name[0])					// check if current directory exist
		{
		 	case SLOT_EMPTY:
		 	case SLOT_DELETED:
				// the directrory pointer of this file points to a deleted or not existen directory
				// therefore no file or subdirectory can be created
		 		return (direntry_exist);
				break;
			default:	// and is a real directory
				if((dir[file->DirectoryIndex].Attribute & ATTR_SUBDIRECTORY) != ATTR_SUBDIRECTORY)
				{	// current file is not a directory therefore no file or subdirectory can be created here
					return (direntry_exist);
				}
				break;
		}
		file->FirstSectorOfFirstCluster = Fat16ClusterToSector(dir[file->DirectoryIndex].StartCluster);
	}

	// update current file data area position to start of first cluster
	file->FirstSectorOfCurrCluster 	= file->FirstSectorOfFirstCluster;
	file->SectorOfCurrCluster		= 0;
	file->ByteOfCurrSector 			= 0;

	do // loop over all data clusters of the current directory entry
	{
		dir_sector = 0; // reset sector counter within a new cluster
		do // loop over all sectors of a cluster or all sectors of the root directory
		{
			curr_sector = file->FirstSectorOfCurrCluster + dir_sector;	// calculate sector number
			file->SectorInCache = curr_sector;							// upate the sector number of file cache.
			if(SD_SUCCESS != SDC_GetSector(file->SectorInCache, file->Cache))// read the sector
			{
				Fat16_Deinit();
				return(direntry_exist);
			}
			dir = (DirEntry_t *)file->Cache;							// set pointer to directory
			// search all directory entries within that sector
			for(dir_entry = 0; dir_entry < DIRENTRIES_PER_SECTOR; dir_entry++)
			{   // check for existing dir entry
				switch((uint8_t)dir[dir_entry].Name[0])
				{
				 	case SLOT_EMPTY:
					case SLOT_DELETED:
						// ignore empty or deleted dir entries
						break;
					default:
						// if existing check attributes before names are compared will safe performance
						if ((dir[dir_entry].Attribute & attribmask) != attribfilter) break; // attribute must match
						// then compare the name to the giveb dirname (first 11 characters include 8 chars of basename and 3 chars extension.)
						i = 0;
						while((i < 11) && (dir[dir_entry].Name[i] == dirname[i])) i++;
						if (i < 10) break; // names does not match
						// if dirname and attribute have matched
						file->Attribute = dir[dir_entry].Attribute; // store attribute of found dir entry
						file->FirstSectorOfFirstCluster = Fat16ClusterToSector(dir[dir_entry].StartCluster); // set sector of first data cluster
						file->FirstSectorOfCurrCluster = file->FirstSectorOfFirstCluster;
						file->SectorOfCurrCluster = 0;
						file->ByteOfCurrSector = 0;
						file->DirectorySector = curr_sector; // current sector
						file->DirectoryIndex  = dir_entry; // current direntry in current sector
						file->Size = dir[dir_entry].Size;
						direntry_exist = 1; // mark as found
						dir_entry = DIRENTRIES_PER_SECTOR;	// stop for-loop
				} // end of first byte of name check
			}
			dir_sector++; // search next sector
		// stop if we reached the end of the cluster or the end of the root dir
		}while((dir_sector < max_dir_sector) && (!direntry_exist));

		// if we are seaching in the data area and the file not found in this cluster so take next cluster.
		if(!direntry_exist && ( Partition.FirstDataSector <= file->FirstSectorOfCurrCluster))
		{
			end_of_directory_not_reached = GetNextCluster(file);  // updates File->FirstSectorOfCurrCluster
		}
	}while((end_of_directory_not_reached) && (!direntry_exist)); // repeat until a next cluster exist an no
	return(direntry_exist);
}


/****************************************************************************************************************************************/
/*	Function: 		CreateDirectoryEntry(int8_t *, uint16_t, File_t *)																			*/
/*																																	  	*/
/*	Description:	This function looks for the next free position in the directory and creates an entry.								*/
/* 					The type of an directory entry is specified by the file attribute.													*/
/*																																	   	*/
/*	Returnvalue: 	Return 0 on error																									*/
/****************************************************************************************************************************************/
uint8_t CreateDirectoryEntry(int8_t *dirname, uint8_t attrib, File_t *file)
{
	uint32_t	dir_sector, max_dir_sector, curr_sector;
	uint16_t	dir_entry	= 0;
	uint16_t	subdircluster, dircluster = 0;
	uint16_t	end_of_directory_not_reached = 0;
	uint8_t		i 			= 0;
	uint8_t		retvalue 	= 0;
	DirEntry_t*	dir;

	if((!Partition.IsValid) || (file == NULL) || (dirname == NULL)) return (retvalue);
	// It is not checked here that the dir entry that should be created is already existent!

	// Dir entries can be created only in file-clusters that have
	// the dir-flag set in its attribute or within the root directory

	file->FirstSectorOfFirstCluster = 0;
	// no current directory exist therefore assume creating in the root
	if(file->DirectorySector == 0)
	{
		max_dir_sector = (Partition.MaxRootEntries * DIRENTRY_SIZE)/BYTES_PER_SECTOR;
		dircluster = 0;
		file->FirstSectorOfFirstCluster = Partition.FirstRootDirSector;
	}
   	// within the root directory area we can read sectors sequentially until the end of this area
	else if((Partition.FirstRootDirSector <= file->DirectorySector) && (file->DirectorySector < Partition.FirstDataSector))
	{
	   	max_dir_sector = (Partition.MaxRootEntries * DIRENTRY_SIZE)/BYTES_PER_SECTOR;
	}
	// within the data clusters we can read sectors sequentially only within the cluster
	else if((Partition.FirstDataSector <= file->DirectorySector) && (file->DirectorySector <= Partition.LastDataSector))
	{
		max_dir_sector = Partition.SectorsPerCluster;
	}
	else return (retvalue); // bad sector range for directory sector of the file
	// if search area is not defined yet
	if(file->FirstSectorOfFirstCluster == 0)
	{
	    // check if the directory entry of current file is existent and has the dir-flag set
		file->SectorInCache = file->DirectorySector;				// update the sector number of file cache.
		if(SD_SUCCESS != SDC_GetSector(file->SectorInCache, file->Cache))// read in the sector.
		{
		 	Fat16_Deinit();
			return(retvalue);
		}
		dir = (DirEntry_t *)file->Cache;							// set pointer to directory
		switch((uint8_t)dir[file->DirectoryIndex].Name[0])					// check if current directory exist
		{
		 	case SLOT_EMPTY:
		 	case SLOT_DELETED:
		 		return (retvalue);
				break;
			default:	// and is a real directory
				if((dir[file->DirectoryIndex].Attribute & ATTR_SUBDIRECTORY) != ATTR_SUBDIRECTORY)
				{	// current file is not a directory therefore no file or subdirectory can be created here
					return (retvalue);
				}
				break;
		}
		dircluster = dir[file->DirectoryIndex].StartCluster;
		file->FirstSectorOfFirstCluster = Fat16ClusterToSector(dircluster);
	}

	subdircluster = FindNextFreeCluster(file);	// get the next free cluster on the disk and mark it as used.
	if(subdircluster)
	{
		file->FirstSectorOfCurrCluster	= file->FirstSectorOfFirstCluster;
		file->SectorOfCurrCluster		= 0;
		do // loop over all clusters of current directory
		{
			dir_sector = 0; // reset sector counter within a new cluster
			do // loop over all sectors of a cluster or all sectors of the root directory
			{
				curr_sector = file->FirstSectorOfCurrCluster + dir_sector;	// calculate sector number
				file->SectorInCache = curr_sector;							// upate the sector number of file cache.
				if(SD_SUCCESS != SDC_GetSector(file->SectorInCache, file->Cache))// read in the sector.
				{
				 	Fat16_Deinit();
					return(retvalue);
				}
				dir = (DirEntry_t *)file->Cache;							// set pointer to directory
				// search all directory entries of a sector
				for(dir_entry = 0; dir_entry < DIRENTRIES_PER_SECTOR; dir_entry++)
				{	// check if current direntry is available
					if(((uint8_t)dir[dir_entry].Name[0] == SLOT_EMPTY) || ((uint8_t)dir[dir_entry].Name[0] == SLOT_DELETED))
					{	// a free direntry was found
						for(i = 0; i < 11; i++) dir[dir_entry].Name[i] = dirname[i];		// Set dir name
						dir[dir_entry].Attribute    = attrib;								// Set the attribute of the new directoryentry.
						dir[dir_entry].StartCluster = subdircluster;						// copy the location of the first datacluster to the directoryentry.
						dir[dir_entry].DateTime 	= FileDateTime(&SystemTime);			// set date/time
						dir[dir_entry].Size     	= 0;									// the new createted file has no content yet.
						if(SD_SUCCESS != SDC_PutSector(file->SectorInCache, file->Cache))	// write back to card
						{
				 			Fat16_Deinit();
							return(retvalue);
						}
						file->FirstSectorOfFirstCluster = Fat16ClusterToSector(subdircluster);	// Calculate absolute sectorposition of first datacluster.
						file->FirstSectorOfCurrCluster  = file->FirstSectorOfFirstCluster;	// Start reading the file with the first sector of the first datacluster.
						file->SectorOfCurrCluster		= 0;								// reset sector of cureen cluster
						file->ByteOfCurrSector 			= 0;								// reset the byte location within the current sector
						file->Attribute 				= attrib;  	    					// set file attribute to dir attribute
						file->Size 						= 0;							    // new file has no size
						file->DirectorySector 			= curr_sector;
						file->DirectoryIndex  			= dir_entry;
						if((attrib & ATTR_SUBDIRECTORY) == ATTR_SUBDIRECTORY) 				// if a new directory was created then initilize the data area
						{
							ClearCurrCluster(file); // fill cluster with zeros
							file->SectorInCache = file->FirstSectorOfFirstCluster;
							if(SD_SUCCESS != SDC_GetSector(file->SectorInCache, file->Cache))// read in the sector.
							{
							 	Fat16_Deinit();
								return(retvalue);
							}
							dir = (DirEntry_t *)file->Cache;
							// create direntry "." to current dir
							dir[0].Name[0] = 0x2E;
							for(i = 1; i < 11; i++) dir[0].Name[i] = ' ';
							dir[0].Attribute = ATTR_SUBDIRECTORY;
							dir[0].StartCluster = subdircluster;
							dir[0].DateTime = 0;
							dir[0].Size = 0;
							// create direntry ".." to the upper dir
							dir[1].Name[0] = 0x2E;
							dir[1].Name[1] = 0x2E;
							for(i = 2; i < 11; i++) dir[1].Name[i] = ' ';
							dir[1].Attribute = ATTR_SUBDIRECTORY;
							dir[1].StartCluster = dircluster;
							dir[1].DateTime = 0;
							dir[1].Size = 0;
							if(SD_SUCCESS != SDC_PutSector(file->SectorInCache, file->Cache))// read in the sector.
							{
							 	Fat16_Deinit();
								return(retvalue);
							}
					   	}
						retvalue = 1;
						dir_entry = DIRENTRIES_PER_SECTOR;	// stop for-loop
					}
				}
				dir_sector++; // search next sector
			// stop if we reached the end of the cluster or the end of the root dir
			}while((dir_sector < max_dir_sector) && (!retvalue));

			// if we are seaching in the data area and the file not found in this cluster so take next cluster.
			if(!retvalue && ( Partition.FirstDataSector <= file->FirstSectorOfCurrCluster))
			{
				end_of_directory_not_reached = GetNextCluster(file);  // updates File->FirstSectorOfCurrCluster
			}
		}while((end_of_directory_not_reached) && (!retvalue));
		// Perhaps we are at the end of the last cluster of a directory file an have no free direntry found.
		// Then we would need to add a cluster to that file and create the new direntry there.
		// This code is not implemented yet, because its occurs only if more that 32*32=1024 direntries are
		// within a subdirectory of root.
	}
	return(retvalue);	// return 1 if file has been created otherwise return 0.
}

/********************************************************************************************************************************************/
/*	Function: 		FileExist(const int8_t* filename, uint8_t attribfilter, uint8_t attribmask, File_t *file);											*/
/*																																	  		*/
/*	Description:	This function looks for the specified file including its subdirectories beginning										*/
/*					in the rootdirectory of the drive. If the file is found the Filepointer properties are									*/
/*					updated.																												*/
/*																																	   		*/
/*	Returnvalue: 	1 if file is found else 0.													   											*/
/********************************************************************************************************************************************/
uint8_t FileExist(const int8_t* filename, const uint8_t attribfilter, const uint8_t attribmask, File_t *file)
{
	int8_t* path = 0;
	int8_t* subpath = 0;
	uint8_t af, am, file_exist = 0;
	int8_t	dirname[12]; // 8+3 + temination character

	// if incomming pointers are useless return immediatly
	if ((filename == NULL) || (file == NULL) || (!Partition.IsValid)) return 0;

	// trace along the filepath
	path = (int8_t*)filename;								// start a the beginning of the filename string
	file->DirectorySector = 0; 								// start at RootDirectory with file search
	file->DirectoryIndex = 0;
	// as long as the file was not found and the remaining path is not empty
	while((*path != 0) && !file_exist)
	{	// separate dirname and subpath from filepath string
		subpath = SeperateDirName(path, dirname);
		if(subpath != NULL)
		{
			if(*subpath == 0)
			{	// empty subpath indicates last element of dir chain
				af = attribfilter;
				am = attribmask;
			}
			else  // it must be a subdirectory and no volume label
			{
			 	af = ATTR_SUBDIRECTORY;
				am = ATTR_SUBDIRECTORY|ATTR_VOLUMELABEL;
			}
			if(!DirectoryEntryExist(dirname, af, am, file))
			{
				return (file_exist); // subdirectory does not exist
			}
			else
			{
				if (*subpath == 0)
				{
					file_exist = 1;	// last element of path chain was found with the given attribute filter
				}
			}
		}
		else // error seperating the subpath
		{
			return file_exist; // bad subdir format
		}
		path = subpath;
		subpath = 0;
	}
	return (file_exist);
}


/********************************************************************************************************************************************/
/*	Function: 		FileCreate(const s8* filename, u8 attrib, File_t *file);																*/
/*																																	  		*/
/*	Description:	This function looks for the specified file including its subdirectories beginning										*/
/*					in the rootdirectory of the partition. If the file is found the Filepointer properties are								*/
/*					updated. If file or its subdirectories are not found they will be created												*/
/*																																	   		*/
/*	Returnvalue: 	1 if file was created else 0.													   										*/
/********************************************************************************************************************************************/
uint8_t FileCreate(const int8_t* filename, const uint8_t attrib, File_t *file)
{
	int8_t *path = 0;
	int8_t *subpath = 0;
	uint8_t af, am, file_created = 0;
	int8_t dirname[12];

	// if incomming pointers are useless return immediatly
	if ((filename == NULL) || (file == NULL) || (!Partition.IsValid)) return 0;

	// trace along the filepath
	path = (int8_t*)filename;									// start a the beginning of the filename string
	file->DirectorySector = 0; 								// start at RootDirectory with file search
	file->DirectoryIndex = 0;
	// as long as the file was not created and the remaining file path is not empty
	while((*path != 0) && !file_created)
	{   // separate dirname and subpath from filepath string
		subpath = SeperateDirName(path, dirname);
		if(subpath != NULL)
		{
			if(*subpath == 0)
			{	// empty subpath indicates last element of dir chain
				af = ATTR_NONE;
				am = ATTR_SUBDIRECTORY|ATTR_VOLUMELABEL;  // any file that is no subdir or volume label
			}
			else  // it must be a subdirectory and no volume label
			{
			 	af = ATTR_SUBDIRECTORY;
				am = ATTR_SUBDIRECTORY|ATTR_VOLUMELABEL;
			}
			if(!DirectoryEntryExist(dirname, af, am, file)) // if subdir or file is not existent
			{  // try to create subdir or file
				if(*subpath == 0) af = attrib; // if last element in dir chain take the given attribute
				if(!CreateDirectoryEntry(dirname, af, file))
				{	// could not be created
				 	return(file_created);
				}
				else if (*subpath == 0) file_created = 1; // last element of path chain was created
			}
		}
		else // error seperating the subpath
		{
			return file_created; // bad subdir format
		}
		path = subpath;
		subpath = 0;
	}
	return (file_created);
}


/********************************************************************************************************************************************/
/*	Function: 		File_t * fopen_(int8_t* filename, int8_t mode);																			 	  	*/
/*																																	  		*/
/*	Description:	This function looks for the specified file in the rootdirectory of the drive. If the file is found the number of the	*/
/*					corrosponding filepointer is returned. Only modes 'r' (reading) and 'a' append are implemented yet.						*/
/*																																	   		*/
/*	Returnvalue: 	The filepointer to the file or 0 if faild.													   							*/
/********************************************************************************************************************************************/
File_t * fopen_(int8_t * const filename, const int8_t mode)
{
	File_t *file	= 0;

	if((!Partition.IsValid) || (filename == 0)) return(file);

	// Look for an unused filepointer in the file pointer list?
	file = LockFilePointer();
	// if no unused file pointer was found return 0
	if(file == NULL) return(file);

	// now we have found a free filepointer and claimed it
	// so let initiate its property values
	file->FirstSectorOfFirstCluster	= 0;		// Sectorpointer to the first sector of the first datacluster of the file.
	file->FirstSectorOfCurrCluster	= 0;		// Pointer to the cluster which is edited at the moment.
	file->SectorOfCurrCluster		= 0;		// The sector which is edited at the moment (cluster_pointer + sector_index).
	file->ByteOfCurrSector 			= 0;		// The bytelocation within the current sector (cluster_pointer + sector_index + byte_index).
	file->Mode 						= mode;		// mode of fileoperation (read,write)
	file->Size	 					= 0;		// the size of the opened file in bytes.
	file->Position	 				= 0;		// pointer to a byte within the file 0 < fileposition < filesize
	file->SectorInCache		 		= 0;		// the last sector read, wich is still in the sectorbuffer.
	file->DirectorySector	 		= 0;		// the sectorposition where the directoryentry has been made.
	file->DirectoryIndex	 		= 0;		// the index to the directoryentry within the specified sector.
	file->Attribute 				= 0;		// the attribute of the file opened.

	// check if a real file (no directory) to the given filename exist
	if(FileExist(filename, ATTR_NONE, ATTR_SUBDIRECTORY|ATTR_VOLUMELABEL, file))
	{  // file exist
		switch(mode)  // check mode
		{
			case 'a':	// if mode is: append to file
				if((file->Attribute & ATTR_READONLY) == ATTR_READONLY)
				{	// file is marked as readonly --> do not open this file
				 	fclose_(file);
					file = NULL;
				}
				else
				{	// file is not marked as read only --> goto end of file
					fseek_(file, 0, SEEK_END);		// point to the end of the file
				}
				break;
			case 'w':	// if mode is: write to file
				if((file->Attribute & ATTR_READONLY) == ATTR_READONLY)
				{	// file is marked as readonly --> do not open this file
					fclose_(file);
					file = NULL;
				}
				else
				{	// file is not marked as read only --> goto start of file
					// free all clusters of that file
					DeleteClusterChain(SectorToFat16Cluster(file->FirstSectorOfFirstCluster));
					// mar an empy cluster as the last one and store the corresponding sector
					file->FirstSectorOfFirstCluster = Fat16ClusterToSector(FindNextFreeCluster(file));
					file->FirstSectorOfCurrCluster = file->FirstSectorOfFirstCluster;
					file->SectorOfCurrCluster = 0;
					file->ByteOfCurrSector = 0;
					file->Size = 0;
					file->Position = 0;
					fseek_(file, 0, SEEK_SET);
				}
				break;
			case 'r':	// if mode is: read from file
				// goto end of file
				fseek_(file, 0, SEEK_SET);
				break;
			default: // other modes are not supported
				fclose_(file);
				file = NULL;
			break;
		}
		return(file);
	}
	else // file does not exist
	{
		switch(mode)  // check mode
		{
		 	case 'a':
			case 'w': // if mode is write or append
				// try to create the file
				if(!FileCreate(filename, ATTR_ARCHIVE, file))
				{ // if it could not be created
					fclose_(file);
					file = NULL;
				}
				break;
			case 'r': // else opened for 'r'
			default:  // of unsupported mode
				fclose_(file);
				file = NULL;
				break;
		}
		return(file);
	}
	// we should never come to this point
	fclose_(file);
	file = NULL;
	return(file);
}

/****************************************************************************************************************************************************/
/* Function: 	fflush_(File *);																							   						*/
/* 																																				   	*/
/* Description:	This function writes the data already in the buffer but not yet written to the file.												*/
/*																																					*/
/* Returnvalue: 0 on success EOF on error																											*/
/****************************************************************************************************************************************************/
int16_t	fflush_(File_t * const file)
{
	DirEntry_t *dir;

	if((file == NULL) || (!Partition.IsValid)) return (EOF);

	switch(file->Mode)
	{
		case 'a':
		case 'w':
			if(file->ByteOfCurrSector > 0)										// has data been added to the file?
			{
				if(SD_SUCCESS != SDC_PutSector(file->SectorInCache, file->Cache))// save the data still in the buffer
				{
					Fat16_Deinit();
				 	return(EOF);
				}
			}
			file->SectorInCache	= file->DirectorySector;
			if(SD_SUCCESS != SDC_GetSector(file->SectorInCache, file->Cache))					// read the directory entry for this file.
			{
				Fat16_Deinit();
				return(EOF);
			}

			dir = (DirEntry_t *)file->Cache;
			dir[file->DirectoryIndex].Size = file->Size;						// update file size
			dir[file->DirectoryIndex].DateTime = FileDateTime(&SystemTime);		// update date time
			if(SD_SUCCESS != SDC_PutSector(file->SectorInCache, file->Cache))	// write back to sd-card
			{
				Fat16_Deinit();
				return(EOF);
			}
			break;
		case 'r':
		default:
			return(EOF);
			break;

	}
	return(0);
}

/****************************************************************************************************************************************/
/*	Function: 		fclose_(File *file);																								*/
/*																																	  	*/
/*	Description:	This function closes the open file by writing the remaining data  													*/
/*					from the buffer to the device and entering the filesize in the directory entry.										*/
/*																																	   	*/
/*	Returnvalue:	0 on success EOF on error																							*/
/****************************************************************************************************************************************/
int16_t fclose_(File_t *file)
{
	int16_t returnvalue = EOF;

	if(file == NULL) return(returnvalue);
	returnvalue = fflush_(file);
	UnlockFilePointer(file);
	return(returnvalue);
}

/********************************************************************************************************************************************/
/*	Function: 		fgetc_(File *file);																			 	  						*/
/*																																	  		*/
/*	Description:	This function reads and returns one character from the specified file. Is the end of the actual sector reached the		*/
/*					next sector of the cluster is read. If the last sector of the cluster read the next cluster will be searched in FAT.	*/
/*																																	   		*/
/*	Returnvalue: 	The function returns the character read from the specified memorylocation as u8 casted to s16 or EOF.					*/
/********************************************************************************************************************************************/
int16_t fgetc_(File_t * const file)
{
	int16_t c = EOF;
	uint32_t curr_sector;

	if( (!Partition.IsValid) || (file == NULL)) return(c);
	// if the end of the file is not reached, get the next character.
	if((0 < file->Size) && ((file->Position+1) < file->Size) )
	{
		curr_sector  = file->FirstSectorOfCurrCluster;		// calculate the sector of the next character to be read.
		curr_sector += file->SectorOfCurrCluster;

		if(file->SectorInCache != curr_sector)
		{
			file->SectorInCache = curr_sector;
			if(SD_SUCCESS != SDC_GetSector(file->SectorInCache,file->Cache))
			{
			 	Fat16_Deinit();
				return(c);
			}
		}
		c = (int16_t) file->Cache[file->ByteOfCurrSector];
		file->Position++;									// increment file position
	   	file->ByteOfCurrSector++;							// goto next byte in sector
		if(file->ByteOfCurrSector >= BYTES_PER_SECTOR)		// if end of sector
		{
			file->ByteOfCurrSector = 0;					   	//  reset byte location
			file->SectorOfCurrCluster++;					//	next sector
			if(file->SectorOfCurrCluster >= Partition.SectorsPerCluster)	// if end of cluster is reached, the next datacluster has to be searched in the FAT.
			{

				if(GetNextCluster(file))										// Sets the clusterpointer of the file to the next datacluster.
				{
					file->SectorOfCurrCluster = 0;								// start reading new cluster at first sector of the cluster.
				}
				else // the last cluster was allready reached
				{
				 	file->SectorOfCurrCluster--;							// jump back to the last sector in the last cluster
					file->ByteOfCurrSector = BYTES_PER_SECTOR;				// set ByteOfCurrSector one byte over sector end
				}
			}
		}
	}
	return(c);
}

/********************************************************************************************************************************************/
/*	Function: 		fputc_( const s8 c, File *file);																			 			*/
/*																																	  		*/
/*	Description:	This function writes a byte to the specified file and takes care of writing the necessary FAT- Entries.					*/
/*					next sector of the cluster is read. If the last sector of the cluster read the next cluster will be searched in FAT.	*/
/*																																	   		*/
/*	Returnvalue: 	The function returns the character written to the stream or EOF on error.												*/
/********************************************************************************************************************************************/
int16_t fputc_(const int8_t c, File_t * const file)
{
	uint32_t curr_sector  = 0;

	if((!Partition.IsValid) || (file == NULL)) return(EOF);

	// If file position equals to file size, then the end of file has reached.
	// In this chase it has to be checked that the ByteOfCurrSector is BYTES_PER_SECTOR
	// and a new cluster should be appended.
	if((file->Position >= file->Size) && (file->ByteOfCurrSector >= BYTES_PER_SECTOR))
	{
		if(!AppendCluster(file)) return(EOF);
	}

	curr_sector  = file->FirstSectorOfCurrCluster;
	curr_sector += file->SectorOfCurrCluster;
	if(file->SectorInCache != curr_sector)
	{
		file->SectorInCache = curr_sector;
		if(SD_SUCCESS != SDC_GetSector(file->SectorInCache, file->Cache))
		{
			Fat16_Deinit();
			return(EOF);
		}
	}

	file->Cache[file->ByteOfCurrSector] = (uint8_t)c;			// write databyte into the buffer. The byte will be written to the device at once
	if(file->Size == file->Position) file->Size++;		// a character has been written to the file so the size is incremented only when the character has been added at the end of the file.
	file->Position++;									// the actual positon within the file.
	file->ByteOfCurrSector++;							// goto next byte in sector
	if(file->ByteOfCurrSector >= BYTES_PER_SECTOR)		// if the end of this sector is reached yet
	{	// save the sector to the sd-card
		if(SD_SUCCESS != SDC_PutSector(file->SectorInCache, file->Cache))
		{
			Fat16_Deinit();
			return(EOF);
		}
		file->ByteOfCurrSector = 0;						//  reset byte location
		file->SectorOfCurrCluster++;					//	next sector
		if(file->SectorOfCurrCluster >= Partition.SectorsPerCluster)// if end of cluster is reached, the next datacluster has to be searched in the FAT.
		{
			if(!GetNextCluster(file))								// Sets the clusterpointer of the file to the next datacluster.
			{ // if current cluster was the last cluster of the file
				if(!AppendCluster(file))						// append a new and free cluster at the end of the file.
				{
					file->SectorOfCurrCluster--; 				// jump back to last sector of last cluster
					file->ByteOfCurrSector = BYTES_PER_SECTOR;	// set byte location to 1 byte over sector len
				 	return(EOF);
				}
			}
			else // next cluster
			{
			 	file->SectorOfCurrCluster = 0;							// start reading new cluster at first sector of the cluster.
			}
		}
	}
	return(0);
}


/****************************************************************************************************************************************/
/*	Function: 		fread_(void *buffer, uint32_t size, uint32_t count, File *File);																*/
/*																																	  	*/
/*	Description:	This function reads count objects of the specified size 															*/
/*					from the actual position of the file to the specified buffer.														*/
/*																																	   	*/
/*	Returnvalue:	The function returns the number of objects (not bytes) read from the file.											*/
/****************************************************************************************************************************************/
uint32_t fread_(void * const buffer, uint32_t size, uint32_t count, File_t * const file)
{
	uint32_t object_cnt 	= 0;											// count the number of objects read from the file.
	uint32_t object_size = 0;											// count the number of bytes read from the actual object.
	uint8_t *pbuff   	= 0;											// a pointer to the actual bufferposition.
	uint8_t success      = 1;											// no error occured during read operation to the file.
	int16_t c;

	if((!Partition.IsValid) || (file == NULL) || (buffer == NULL)) return(0);

	pbuff = (uint8_t *) buffer;											// cast the void pointer to an u8 *

	while((object_cnt < count) && success)
	{
		object_size = size;
		while((size > 0) && success)
		{
			c = fgetc_(file);
			if(c != EOF)
			{
				*pbuff = (uint8_t)c; 									// read a byte from the buffer to the opened file.
				pbuff++;
				size--;
			}
			else // error or end of file reached
			{
			 	success = 0;
			}
		}
		if(success) object_cnt++;
	}
	return(object_cnt);												// return the number of objects succesfully read from the file
}


/****************************************************************************************************************************************/
/*	Function: 		fwrite_(void *buffer, uint32_t size, uint32_t count, File *file);																*/
/*																																	  	*/
/*	Description:	This function writes count objects of the specified size 															*/
/*					from the buffer pointer to the actual position in the file.															*/
/*																																	   	*/
/*	Returnvalue:	The function returns the number of objects (not bytes) read from the file.											*/
/****************************************************************************************************************************************/
uint32_t fwrite_(void * const buffer, uint32_t size, uint32_t count, File_t * const file)
{
	uint32_t object_cnt 	= 0;														// count the number of objects written to the file.
	uint32_t object_size = 0;														// count the number of bytes written from the actual object.
	uint8_t *pbuff	    = 0;														// a pointer to the actual bufferposition.
	uint8_t success      = 1;														// no error occured during write operation to the file.
	int16_t c;

	if((!Partition.IsValid) || (file == NULL) || (buffer == NULL)) return(0);

	pbuff = (uint8_t *) buffer;														// cast the void pointer to an u8 *

	while((object_cnt < count) && success)
	{
		object_size = size;
		while((size > 0) && success)
		{
			c = fputc_(*pbuff, file);										// write a byte from the buffer to the opened file.
			if(c != EOF)
			{
				pbuff++;
				size--;
			}
			else
			{
			 	success = 0;
			}
		}
		if(success) object_cnt++;
	}

	return(object_cnt);																// return the number of objects succesfully written to the file
}


/****************************************************************************************************************************************/
/*	Function: 		fputs_(const int8_t *string, File_t *File);																				*/
/*																																	  	*/
/*	Description:	This function writes a string to the specified file. 																*/
/*																																	   	*/
/*	Returnvalue:	The function returns a no negative value or EOF on error.															*/
/****************************************************************************************************************************************/
int16_t fputs_(int8_t * const string, File_t * const file)
{
	uint8_t i=0;
	int16_t c = 0;

	if((!Partition.IsValid) || (file == NULL) || (string == NULL)) return(0);

	while((string[i] != 0)&& (c != EOF))
	{
		c = fputc_(string[i], file);
		i++;
	}
	return(c);
}

/****************************************************************************************************************************************/
/*	Function: 		fgets_(int8 *, int16_t , File_t *);																						*/
/*																																	  	*/
/*	Description:	This function reads a string from the file to the specifies string. 												*/
/*																																	   	*/
/*	Returnvalue:	A pointer to the string read from the file or 0 on error.															*/
/****************************************************************************************************************************************/
int8_t * fgets_(int8_t * const string, int16_t length, File_t * const file)
{
	int8_t *pbuff;
	int16_t c = 0, bytecount;

	if( (!Partition.IsValid) || (file == NULL) || (string == NULL) || (length < 1) ) return (0);

	pbuff = string;
	bytecount = length;
	while(bytecount > 1)							// read the count-1 characters from the file to the string.
	{
		c = fgetc_(file);							// read a character from the opened file.

		switch(c)
		{
			case 0x0A:
				*pbuff = 0;							// set string terminator
				return(string);				 		// stop loop
				break;

			case EOF:
				*pbuff = 0;							// set string terminator
				return(0);
			default:
				*pbuff++ = (int8_t)c;				// copy byte to string
				bytecount--;
				break;
		}
	}
	*pbuff = 0;
	return(string);
}

/****************************************************************************************************************************************/
/*	Function: 		fexist_(const int8_t*);																									*/
/*																																	  	*/
/*	Description:	This function checks if a file already exist.																		*/
/*																																	   	*/
/*	Returnvalue:	1 if the file exist else 0.																							*/
/****************************************************************************************************************************************/
uint8_t fexist_(int8_t* const filename)
{
	uint8_t exist = 0;
	File_t *file = 0;
	file = LockFilePointer();
	exist = FileExist(filename, ATTR_NONE, ATTR_SUBDIRECTORY|ATTR_VOLUMELABEL, file);
	UnlockFilePointer(file);
	return(exist);
}

/****************************************************************************************************************************************/
/*	Function: 		feof_(File_t *File);																								*/
/*																																	  	*/
/*	Description:	This function checks wether the end of the file has been reached.																		*/
/*																																	   	*/
/*	Returnvalue:	0 if the end of the file was not reached otherwise 1.																						*/
/****************************************************************************************************************************************/
uint8_t feof_(File_t *file)
{
	if(((file->Position)+1) < (file->Size))
	{
		return(0);
	}
	else
	{
		return(1);
	}
}


