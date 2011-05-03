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
#include <string.h>
#include "gpx.h"
#include "gpx_header.h"
#include "timer0.h"
#include "main.h"

//________________________________________________________________________________________________________________________________________
// Function: 	GPX_DocumentInit(GPX_Document_t *)
//
// Description:	This function initializes the gpx-document for further use.
//
//
// Returnvalue: '1' if document was initialized
//________________________________________________________________________________________________________________________________________

uint8_t GPX_DocumentInit(GPX_Document_t *doc)
{
	if(doc->state != GPX_DOC_CLOSED) GPX_DocumentClose(doc);						// close file if it was opened before
	doc->state	 = GPX_DOC_CLOSED;													// init state of the gpx-document
	doc->file	 = NULL;
	return(1);
}

//________________________________________________________________________________________________________________________________________
// Function: 	GPX_Document_Open(s8 *name, GPX_Document_t *doc);
//
// Description:	This function opens a new gpx-document with the specified name and creates the document header within the file.
//
//
// Returnvalue: '1' if the gpx-file could be created.
//________________________________________________________________________________________________________________________________________

uint8_t GPX_DocumentOpen(int8_t *name, GPX_Document_t *doc)
{

	uint8_t retvalue = 0;
	uint16_t i;
	int8_t c;
	const prog_char *str;

	if(doc == NULL) return(0);
	GPX_DocumentInit(doc);														// intialize the document with resetvalues
	doc->file = fopen_(name,'a');												// open a new file with the specified filename on the memorycard.

	if(doc->file != NULL)														// could the file be opened?
	{
		retvalue = 1;															// the document could be created on the drive.
		doc->state = GPX_DOC_OPENED;											// change document state to opened. At next a placemark has to be opened.
		str = GPX_DOCUMENT_HEADER;												// write the gpx-header to the document..
		for(i= 0; i < sizeof(GPX_DOCUMENT_HEADER)-1; i++)
		{
			c = (int8_t)pgm_read_byte(str++); // get byte from flash
			fputc_(c, doc->file); // and write that to sd-card
		}
	}

	return(retvalue);
}

//________________________________________________________________________________________________________________________________________
// Function: 	DocumentClose(GPX_Document_t *doc);
//
// Description:	This function closes the document specified by doc.
//
//
// Returnvalue: '1' if the gpx-file could be closed.
//________________________________________________________________________________________________________________________________________

uint8_t GPX_DocumentClose(GPX_Document_t *doc)
{

	uint8_t retvalue = 1;
	uint16_t i;
	int8_t c;
	const prog_char *str;

	if(doc == NULL) return(0);

	while(doc->state != GPX_DOC_CLOSED)								// close linestring, placemark and document before closing the file on the memorycard
	{
		switch(doc->state)
		{
			case GPX_DOC_TRACKSEGMENT_OPENED:
				GPX_TrackSegmentEnd(doc);						// write terminating tag to end tracksegment.
				break;

			case GPX_DOC_TRACK_OPENED:								// write terminating tag to close track.
				GPX_TrackEnd(doc);
				break;

			case GPX_DOC_OPENED:									// close the file on the memorycard
				if(doc->file != NULL)
				{
					str = GPX_DOCUMENT_FOOTER;						// write the gpx-footer to the document.
					for(i= 0; i < sizeof(GPX_DOCUMENT_FOOTER)-1; i++)
					{
						c = (int8_t)pgm_read_byte(str++); // get byte from flash
						fputc_(c, doc->file); // and write that to sd-card
					}
					fclose_(doc->file);
					retvalue = 1;
				}
				doc->state = GPX_DOC_CLOSED;
				break;

			default:
				doc->state = GPX_DOC_CLOSED;
				break;
		}
	}
	return(retvalue);
}

//________________________________________________________________________________________________________________________________________
// Function: 	u8 GPX_TrackBegin(GPX_Document_t);
//
// Description:	This function adds a track to the document.
//
//
// Returnvalue: '1' if the track could be opened
//________________________________________________________________________________________________________________________________________

uint8_t GPX_TrackBegin(GPX_Document_t *doc)
{

	uint8_t retvalue = 0;
	uint16_t i;
	int8_t c;
	const prog_char *str;


	if(doc->state == GPX_DOC_OPENED)
	{
		if(doc->file != NULL)
		{
			doc->state = GPX_DOC_TRACK_OPENED;
			retvalue = 1;
			str = GPX_TRACK_HEADER;
			for(i= 0; i < sizeof(GPX_TRACK_HEADER)-1; i++)
			{
				c = (int8_t)pgm_read_byte(str++); // get byte from flash
				fputc_(c, doc->file); // and write that to sd-card
			}
		}
	}
	return(retvalue);
}

//________________________________________________________________________________________________________________________________________
// Function: 	u8 GPX_TrackEnd(KML_Document_t *doc)
//
// Description:	This function ends the track opened before.
//
//
// Returnvalue: 1' if the track could be closed
//________________________________________________________________________________________________________________________________________

uint8_t GPX_TrackEnd(GPX_Document_t *doc)
{

	uint8_t retvalue = 0;
	uint16_t i;
	int8_t c;
	const prog_char *str;

	if(doc->state == GPX_DOC_TRACK_OPENED)
	{
		if(doc->file != NULL)
		{
			doc->state = GPX_DOC_OPENED;
			str = GPX_TRACK_FOOTER;
			for(i= 0; i < sizeof(GPX_TRACK_FOOTER)-1; i++)
			{
				c = (int8_t)pgm_read_byte(str++); // get byte from flash
				fputc_(c, doc->file); // and write that to sd-card
			}
		}
	}

	return(retvalue);
}

//________________________________________________________________________________________________________________________________________
// Function: 	u8 GPX_TrackSegmentBegin(GPX_Document_t *doc);
//
// Description:	This function starts a track segment.
//
//
// Returnvalue: '1' if the track segement could be started
//________________________________________________________________________________________________________________________________________

uint8_t GPX_TrackSegmentBegin(GPX_Document_t *doc)
{

	uint8_t retvalue = 0;
	uint16_t i;
	int8_t c;
	const prog_char *str;

	if(doc->state == GPX_DOC_TRACK_OPENED)
	{
		if(doc->file != NULL)
		{
			doc->state = GPX_DOC_TRACKSEGMENT_OPENED;
			str = GPX_TRACKSEGMENT_HEADER;
			for(i = 0; i < sizeof(GPX_TRACKSEGMENT_HEADER)-1; i++)
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
// Function: 	u8 GPX_TrackSegmentEnd(GPX_Document_t *doc);
//
// Description:	This function ends the tracksegment opened before.
//
//
// Returnvalue: '1' if the track segment could be terminated
//________________________________________________________________________________________________________________________________________

uint8_t GPX_TrackSegmentEnd(GPX_Document_t *doc)
{

	uint8_t retvalue = 0;
	uint16_t i;
	int8_t c;
	const prog_char *str;


	if(doc->state == GPX_DOC_TRACKSEGMENT_OPENED)
	{
		if(doc->file != NULL)
		{
			doc->state = GPX_DOC_TRACK_OPENED;
			str = GPX_TRACKSEGMENT_FOOTER;
			for(i = 0; i < sizeof(GPX_TRACKSEGMENT_FOOTER)-1; i++)
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
// Function: 	u8 GPX_TrackSegementAddPoint(GPS_Pos_t * pGPS_Position ,GPX_Document_t *doc);
//
// Description:	This function adds a pointof a track segement to the specified document.
//
//
// Returnvalue: '1' if a point was added
//________________________________________________________________________________________________________________________________________

uint8_t GPX_TrackSegementAddPoint(GPX_Document_t *doc)
{

	uint8_t retvalue = 0;
	int8_t string[100];

	if(doc == NULL) return(0);

    if(GPSData.Position.Status != INVALID)
	{
		if(doc->state == GPX_DOC_TRACKSEGMENT_OPENED)
		{
			if(doc->file != NULL)
			{
				int16_t i16_1, i16_2, i16_3;
				uint8_t u8_1, u8_2;
				// write <trkpt> tag
				if(GPSData.Position.Latitude < 0) u8_1 = '-';
				else u8_1 = '+';
				i16_1 = abs((int16_t)(GPSData.Position.Latitude/10000000L));
				i16_2 = abs((int16_t)((GPSData.Position.Latitude%10000000L)/10000L));
				i16_3 = abs((int16_t)(((GPSData.Position.Latitude%10000000L)%10000L)/10L));
				sprintf(string, "<trkpt lat=\"%c%d.%.3d%.3d\" ",u8_1, i16_1, i16_2, i16_3);
				fputs_(string, doc->file);

				if(GPSData.Position.Longitude < 0) u8_1 = '-';
				else u8_1 = '+';
				i16_1 = abs((int16_t)(GPSData.Position.Longitude/10000000L));
				i16_2 = abs((int16_t)((GPSData.Position.Longitude%10000000L)/10000L));
				i16_3 = abs((int16_t)(((GPSData.Position.Longitude%10000000L)%10000L)/10L));
				sprintf(string, "lon=\"%c%d.%.3d%.3d\" >\r\n",u8_1, i16_1, i16_2, i16_3);
				fputs_(string, doc->file);

				// write <time> tag	only at a resolution of one second
				sprintf(string, "<time>%04d-%02d-%02dT%02d:%02d:%02dZ</time>\r\n",SystemTime.Year, SystemTime.Month, SystemTime.Day, SystemTime.Hour, SystemTime.Min, SystemTime.Sec);
				fputs_(string, doc->file);
				// write <sat> tag
				sprintf(string, "<sat>%d</sat>\r\n", GPSData.NumOfSats);
				fputs_(string, doc->file);
				// todo: add  <extensions> tag with additional data to be logged
				sprintf(string, "<extensions>\r\n");
				fputs_(string, doc->file);
				// Course in deg
				i16_1 = (int16_t)(GPSData.Heading/100000L);
				sprintf(string, "<Course>%03d</Course>\r\n", i16_1);
				fputs_(string, doc->file);
				// Ground Speed in cm/s
				sprintf(string, "<GroundSpeed>%d</GroundSpeed>\r\n", (uint16_t)GPSData.Speed_Ground);
				fputs_(string, doc->file);
				// Ubat
				u8_1 = UBat / 10;
				u8_2 = UBat % 10;
 				sprintf(string, "<Voltage>%d.%01d</Voltage>\r\n", u8_1, u8_2);
				fputs_(string, doc->file);

				// eof extensions
				sprintf(string, "</extensions>\r\n");
				fputs_(string, doc->file);
				sprintf(string, "</trkpt>\r\n");
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
// Description:	This function opens and adds gpscoordinates to an GPX-Document. The document will be opened, if not already done
//
//
// Returnvalue: '1' if an gps coordinate was logged
//________________________________________________________________________________________________________________________________________

uint8_t GPX_LoggGPSCoordinates(GPX_Document_t *doc)
{
	uint8_t retval = 0;
	while(doc->state != GPX_DOC_TRACKSEGMENT_OPENED)				// automatic create document with default filename on the card.
	{
		switch(doc->state)
		{
	 		case GPX_DOC_CLOSED:									// document hasn't been opened yet therefore it will be initialized automatically
				retval = GPX_DocumentOpen("default.gpx",doc);	// open the gpx-document with a standardname.
			break;

			case GPX_DOC_OPENED:									// if a document has been opened before but no track exists:
		   		retval = GPX_TrackBegin(doc);
			break;

			case GPX_DOC_TRACK_OPENED:								// add tracksegement to the track
				retval = GPX_TrackSegmentBegin(doc);
			break;

			default:
				retval = 0;
			break;

		}
		if(retval != 1) return(retval); // stop on error
	}

	if(doc->state == GPX_DOC_TRACKSEGMENT_OPENED)						// if the document was opened add coordinates to the document.
	{
		retval = GPX_TrackSegementAddPoint(doc);						// add a track segment point
	}
	return(retval);
}

