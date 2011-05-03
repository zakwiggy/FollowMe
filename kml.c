/*#######################################################################################*/
/* !!! THIS IS NOT FREE SOFTWARE !!!  	                                                 */
/*#######################################################################################*/
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// + Copyright (c) 2008 Ingo Busker, Holger Buss
// + Nur für den privaten Gebrauch
// + FOR NON COMMERCIAL USE ONLY
// + www.MikroKopter.com
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// + Es gilt für das gesamte Projekt (Hardware, Software, Binärfiles, Sourcecode und Dokumentation),
// + dass eine Nutzung (auch auszugsweise) nur für den privaten (nicht-kommerziellen) Gebrauch zulässig ist.
// + Sollten direkte oder indirekte kommerzielle Absichten verfolgt werden, ist mit uns (info@mikrokopter.de) Kontakt
// + bzgl. der Nutzungsbedingungen aufzunehmen.
// + Eine kommerzielle Nutzung ist z.B.Verkauf von MikroKoptern, Bestückung und Verkauf von Platinen oder Bausätzen,
// + Verkauf von Luftbildaufnahmen, usw.
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// + Werden Teile des Quellcodes (mit oder ohne Modifikation) weiterverwendet oder veröffentlicht,
// + unterliegen sie auch diesen Nutzungsbedingungen und diese Nutzungsbedingungen incl. Copyright müssen dann beiliegen
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// + Sollte die Software (auch auszugesweise) oder sonstige Informationen des MikroKopter-Projekts
// + auf anderen Webseiten oder sonstigen Medien veröffentlicht werden, muss unsere Webseite "http://www.mikrokopter.de"
// + eindeutig als Ursprung verlinkt werden
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// + Keine Gewähr auf Fehlerfreiheit, Vollständigkeit oder Funktion
// + Benutzung auf eigene Gefahr
// + Wir übernehmen keinerlei Haftung für direkte oder indirekte Personen- oder Sachschäden
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// + Die PORTIERUNG der Software (oder Teile davon) auf andere Systeme (ausser der Hardware von www.mikrokopter.de) ist nur
// + mit unserer Zustimmung zulässig
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// + Die Funktion printf_P() unterliegt ihrer eigenen Lizenz und ist hiervon nicht betroffen
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// + Redistributions of source code (with or without modifications) must retain the above copyright notice,
// + this list of conditions and the following disclaimer.
// +   * Neither the name of the copyright holders nor the names of contributors may be used to endorse or promote products derived
// +     from this software without specific prior written permission.
// +   * The use of this project (hardware, software, binary files, sources and documentation) is only permitted
// +     for non-commercial use (directly or indirectly)
// +     Commercial use (for excample: selling of MikroKopters, selling of PCBs, assembly, ...) is only permitted
// +     with our written permission
// +   * If sources or documentations are redistributet on other webpages, out webpage (http://www.MikroKopter.de) must be
// +     clearly linked as origin
// +   * PORTING this software (or part of it) to systems (other than hardware from www.mikrokopter.de) is NOT allowed
//
// +  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// +  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// +  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// +  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// +  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// +  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// +  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// +  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// +  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// +  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// +  POSSIBILITY OF SUCH DAMAGE.
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#include <stdlib.h>
#include <stdio.h>
#include "kml.h"
#include "kml_header.h"


//________________________________________________________________________________________________________________________________________
// Module name:			kml.c
// Compiler used:		avr-gcc 3.4.5
// Last Modifikation:	22.03.2009
// Version:				1.03
// Authors:				Stephan Busker, Gregor Stobrawa
// Description:			Source files to write gps-coordinates to a file in the kml (keyhole markup language) fileformat
//						Copyright (C) 2007 Stephan Busker
//........................................................................................................................................
// Functions:			extern u8	KML_LoggGPSCoordinates(struct str_gps_nav_data , KML_Document_t *);	// intializes the kml-document with standard filename and adds points to the file
//						extern u8 	KML_DocumentInit(KML_Document_t *doc)								// initializes the kml-document to resetvalues.
//						extern u8 	KML_DocumentOpen(s8 *, KML_Document_t *);							// opens a new kml document. A filename can be specified.
//						extern u8 	KML_DocumentClose(KML_Document_t *doc);								// closes an open document
//						extern u8   KML_PlaceMarkOpen(KML_Document_t *);								// opens a new placemark within the specified document
//						extern u8 	KML_PlaceMarkClose(KML_Document_t *);								// Closes the placemark
//						extern u8 	KML_LineStringBegin(KML_Document_t *);								// begins a new line within the actual placemark
//						extern u8 	KML_LineStringEnd(KML_Document_t *doc);								// ends the actual linestring
//						extern u8	KML_LineStringAddPoint(struct str_gps_nav_data, KML_Document_t *);	// adds a new point (gps-coordinates) to the actual linestring
//........................................................................................................................................
// ext. functions:
//
//........................................................................................................................................
//
// URL:					www.Mikro-Control.de
// mailto:				stephan.busker@mikro-control.de
//________________________________________________________________________________________________________________________________________



//________________________________________________________________________________________________________________________________________
// Function: 	KML_DocumentInit(KML_Document_t *)
//
// Description:	This function initializes the kml-document for further use.
//
//
// Returnvalue: '1' if document was initialized
//________________________________________________________________________________________________________________________________________

uint8_t KML_DocumentInit(KML_Document_t *doc)
{
	doc->state	 = KML_DOC_CLOSED;													// state of the kml-document
	doc->file	 = NULL;
	return(1);
}

//________________________________________________________________________________________________________________________________________
// Function: 	KML_Document_Open(void);
//
// Description:	This function opens a new KML- document with the specified name and creates the document header within the file.
//
//
// Returnvalue: '1' if the KML- file could be created.
//________________________________________________________________________________________________________________________________________

uint8_t KML_DocumentOpen(int8_t *name, KML_Document_t *doc)
{

	uint8_t retvalue = 0;
	uint16_t i;
	int8_t c;
	const prog_char *str;

	if(doc == NULL) return(0);

	KML_DocumentInit(doc);														// intialize the document with resetvalues
	doc->file = fopen_(name,'a');												// open a new file with the specified filename on the memorycard.

	if(doc->file != NULL)														// could the file be opened?
	{
		retvalue = 1;															// the document could be created on the drive.
		doc->state = KML_DOC_OPENED;											// change document state to opened. At next a placemark has to be opened.
		str = KML_DOCUMENT_HEADER;												// write the KML-header to the document.
		for(i= 0; i < sizeof(KML_DOCUMENT_HEADER)-1; i++)
		{
			c = (int8_t)pgm_read_byte(str++); // get byte from flash
			fputc_(c, doc->file); // and write that to sd-card
		}
	}
	return(retvalue);
}

//________________________________________________________________________________________________________________________________________
// Function: 	DocumentClose(KML_Document_t *doc);
//
// Description:	This function closes the document specified by doc.
//
//
// Returnvalue: '1' if the KML- file could be closed.
//________________________________________________________________________________________________________________________________________

uint8_t KML_DocumentClose(KML_Document_t *doc)
{

	uint8_t retvalue = 1;
	uint16_t i;
	int8_t c;
	const prog_char *str;

	if(doc == NULL) return(0);

	while(doc->state != KML_DOC_CLOSED)								// close linestring, placemark and document before closing the file on the memorycard
	{
		switch(doc->state)
		{
			case KML_DOC_LINESTRING_OPENED:
				KML_LineStringEnd(doc);							// write terminating tag to end linestring.
				break;

	 		case KML_DOC_PLACEMARK_OPENED:							// write terminating tag to close placemark.
				KML_PlaceMarkClose(doc);
				break;

			case KML_DOC_OPENED:									// close the file on the memorycard
				if(doc->file != NULL)
				{
					str = KML_DOCUMENT_FOOTER;						// write the KML- footer to the document.
					for(i= 0; i < sizeof(KML_DOCUMENT_FOOTER)-1; i++)
					{
						c = (int8_t)pgm_read_byte(str++); // get byte from flash
						fputc_(c, doc->file); // and write that to sd-card
					}
					fclose_(doc->file);
					retvalue = 1;
				}
				doc->state = KML_DOC_CLOSED;
				break;

			default:
				doc->state = KML_DOC_CLOSED;
				break;

		}
	}
	return(retvalue);
}

//________________________________________________________________________________________________________________________________________
// Function: 	u8 KML_PlaceMarkOpen(KML_Document_t *doc);
//
// Description:	This function adds a placemark to the document.
//
//
// Returnvalue: '1' if the PlaceMark could be opened
//________________________________________________________________________________________________________________________________________

uint8_t KML_PlaceMarkOpen(KML_Document_t *doc)
{
	uint8_t retvalue = 0;
	uint16_t i;
	int8_t c;
	const prog_char *str;

   	if(doc->state == KML_DOC_OPENED)
	{
		if(doc->file != NULL)
		{
			doc->state = KML_DOC_PLACEMARK_OPENED;
			retvalue = 1;
			str = KML_PLACEMARK_HEADER;
			for(i= 0; i < sizeof(KML_PLACEMARK_HEADER)-1; i++)
			{
				c = (int8_t)pgm_read_byte(str++); // get byte from flash
				fputc_(c, doc->file); // and write that to sd-card
			}
		}
	}
	return(retvalue);
}

//________________________________________________________________________________________________________________________________________
// Function: 	u8 PlaceMarkClose(KML_PlaceMark_t *place, File *file);
//
// Description:	This function ends the placemark opened before.
//
//
// Returnvalue: 1' if the PlaceMark could be closed
//________________________________________________________________________________________________________________________________________

uint8_t KML_PlaceMarkClose(KML_Document_t *doc)
{

	uint8_t retvalue = 0;
	uint16_t i;
	int8_t c;
	const prog_char *str;

	if(doc->state == KML_DOC_PLACEMARK_OPENED)
	{
		if(doc->file != NULL)
		{
			doc->state = KML_DOC_OPENED;
			str = KML_PLACEMARK_FOOTER;
			for(i= 0; i < sizeof(KML_PLACEMARK_FOOTER)-1; i++)
			{
				c = (int8_t)pgm_read_byte(str++); // get byte from flash
				fputc_(c, doc->file); // and write that to sd-card
			}
			retvalue = 1;
		}
	}
	return(retvalue);
}

//________________________________________________________________________________________________________________________________________
// Function: 	u8 LineStringBegin(KML_Document_t *doc);
//
// Description:	This function ends the placemark opened before.
//
//
// Returnvalue: '1' if the LineString could be started
//________________________________________________________________________________________________________________________________________

uint8_t KML_LineStringBegin(KML_Document_t *doc)
{

	uint8_t retvalue = 0;
	uint16_t i;
	int8_t c;
	const prog_char *str;

	if(doc->state == KML_DOC_PLACEMARK_OPENED)
	{
		if(doc->file != NULL)
		{
			doc->state = KML_DOC_LINESTRING_OPENED;
			str = KML_LINESTRING_HEADER;
			for(i= 0; i < sizeof(KML_LINESTRING_HEADER)-1; i++)
			{
				c = (int8_t)pgm_read_byte(str++); // get byte from flash
				fputc_(c, doc->file); // and write that to sd-card
			}
			retvalue = 1;
		}
	}
	return(retvalue);
}

//________________________________________________________________________________________________________________________________________
// Function: 	u8 KML_LineStringEnd(KML_Document_t *doc)
//
// Description:	This function ends the placemark opened before.
//
//
// Returnvalue: '1' if the LineString could be terminated
//________________________________________________________________________________________________________________________________________

uint8_t KML_LineStringEnd(KML_Document_t *doc)
{

	uint8_t retvalue = 0;
	uint16_t i;
	int8_t c;
	const prog_char *str;

	if(doc->state == KML_DOC_LINESTRING_OPENED)
	{
		if(doc->file != NULL)
		{
			doc->state = KML_DOC_PLACEMARK_OPENED;
			str = KML_LINESTRING_FOOTER;
			for(i= 0; i < sizeof(KML_LINESTRING_FOOTER)-1; i++)
			{
				c = (int8_t)pgm_read_byte(str++); // get byte from flash
				fputc_(c, doc->file); // and write that to sd-card
			}
			retvalue = 1;
		}
	}
	return(retvalue);
}

//________________________________________________________________________________________________________________________________________
// Function: 	u8 LineStringAddPoint(gps_data_t, KML_Document_t *doc)
//
// Description:	This function adds a point to the specified document.
//
//
// Returnvalue: '1' if a ppoint was added could be started
//________________________________________________________________________________________________________________________________________

uint8_t KML_LineStringAddPoint(KML_Document_t *doc)
{

	uint8_t retvalue = 0;
	int8_t string[50];

	if(doc == NULL) return(0);

	if(GPSData.Position.Status != INVALID)
	{
		if(doc->state == KML_DOC_LINESTRING_OPENED)
		{
			if(doc->file != NULL)
			{
				int16_t i1, i2, i3;
				uint8_t sign;
				if(GPSData.Position.Longitude < 0) sign = '-';
				else sign = '+';
				i1 = abs((int16_t)(GPSData.Position.Longitude/10000000L));
				i2 = abs((int16_t)((GPSData.Position.Longitude%10000000L)/10000L));
				i3 = abs((int16_t)(((GPSData.Position.Longitude%10000000L)%10000L)/10L));
				sprintf(string,"\r\n%c%d.%.3d%.3d,",sign, i1, i2, i3);
				fputs_(string, doc->file);
				if(GPSData.Position.Latitude < 0) sign = '-';
				else sign = '+';
				i1 = abs((int16_t)(GPSData.Position.Latitude/10000000L));
				i2 = abs((int16_t)((GPSData.Position.Latitude%10000000L)/10000L));
				i3 = abs((int16_t)(((GPSData.Position.Latitude%10000000L)%10000L)/10L));
				sprintf(string,"%c%d.%.3d%.3d,",sign, i1, i2, i3);
				fputs_(string, doc->file);
				sign = '+';
				sprintf(string,"%c%d.%.3d",sign, 0, 0);
				fputs_(string, doc->file);
				retvalue = 1;
			}
		}
	}
	return(retvalue);
}

//________________________________________________________________________________________________________________________________________
// Function: 	u8 KML_LoggGPSCoordinates(gps_data_t *, KML_Document_t *)
//
// Description:	This function opens adds gpscoordinates to an KML-Document. The document will be opened, if not already done
//
//
// Returnvalue: '1' if an gps coordinate was logged
//________________________________________________________________________________________________________________________________________

uint8_t KML_LoggGPSCoordinates(KML_Document_t *doc)
{
	uint8_t retval = 0;
	while(doc->state != KML_DOC_LINESTRING_OPENED)					// automatic create document with default filename on the card.
	{
		switch(doc->state)
		{
	 		case KML_DOC_CLOSED:									// document hasn't been opened yet therefore it will be initialized automatically
				retval = KML_DocumentOpen("default.kml",doc);		// open the kml-document with a standardname.
			break;

			case KML_DOC_OPENED:									// if a document has been opened before but no placemark exists:
		   		retval = KML_PlaceMarkOpen(doc);
			break;

			case KML_DOC_PLACEMARK_OPENED:							// add linestring to the placemark
				retval = KML_LineStringBegin(doc);
			break;

			default:
				retval = 0;
			break;

		}
		if(retval != 1) return(retval); // stop on error
	}

	if(doc->state == KML_DOC_LINESTRING_OPENED)						// if the document was opened add coordinates to the document.
	{
		retval = KML_LineStringAddPoint(doc);
	}
	return(retval);
}

