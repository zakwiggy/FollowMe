#ifndef _FAT16_H
#define _FAT16_H


//________________________________________________________________________________________________________________________________________
//
// Definitions
//
//________________________________________________________________________________________________________________________________________

//#define		__USE_TIME_DATE_ATTRIBUTE
#define	FILE_MAX_OPEN	3				// The number of files that can accessed simultaneously.
#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2
#define	EOF	(-1)
#define BYTES_PER_SECTOR	512
/*
________________________________________________________________________________________________________________________________________

	Structure of a filepointer
________________________________________________________________________________________________________________________________________
*/
typedef struct
{
	uint32_t	FirstSectorOfFirstCluster;	// First sector of the first cluster of the file.
	uint32_t	FirstSectorOfCurrCluster;	// First sector of the cluster which is edited at the moment.
	uint8_t		SectorOfCurrCluster;		// The sector within the current cluster.
	uint16_t	ByteOfCurrSector;			// The byte location within the current sector.
	uint8_t		Mode;						// Mode of fileoperation (read,write)
	uint32_t	Size;						// The size of the opend file in bytes.
	uint32_t	Position;					// Pointer to a character within the file 0 < fileposition < filesize
	uint32_t	DirectorySector;			// the sectorposition where the directoryentry has been made.
	uint16_t	DirectoryIndex;				// The index to the directoryentry within the specified sector.
	uint8_t 	Attribute;					// The attribute of the file opened.
	uint8_t		Cache[BYTES_PER_SECTOR];	// Cache for read and write operation from or to the sd-card.
	uint32_t	SectorInCache;				// The last sector read, which is still in the sector cache.
	uint8_t		State;						// State of the filepointer (used/unused/...)
} File_t;

//________________________________________________________________________________________________________________________________________
//
// API to the FAT16 filesystem
//
//________________________________________________________________________________________________________________________________________

extern uint8_t		Fat16_Init(void);
extern uint8_t		Fat16_Deinit(void);
extern uint8_t		Fat16_IsValid(void);

extern File_t *		fopen_(int8_t * const filename, const int8_t mode);
extern int16_t 		fclose_(File_t *file);
extern uint8_t		fexist_(int8_t * const filename);
extern int16_t		fflush_(File_t * const file);
extern int16_t  	fseek_(File_t * const file, int32_t offset, int16_t origin);
extern int16_t		fgetc_(File_t * const file);
extern int16_t		fputc_(const int8_t c, File_t * const file);
extern uint32_t		fread_(void * const buffer, uint32_t size, uint32_t count, File_t * const file);
extern uint32_t		fwrite_(void *buffer, uint32_t size, uint32_t count, File_t *file);
extern int16_t		fputs_(int8_t * const string, File_t * const file);
extern int8_t *  	fgets_(int8_t * const string, const int16_t length, File_t * const file);
extern uint8_t 		feof_(File_t * const file);



#endif //_FAT16_H




