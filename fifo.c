#include "fifo.h"
#include <avr/io.h>
unsigned char fifo_init (fifo_t* f, unsigned char* buffer, const unsigned int size)
{
	//if(f == NULL) return(0);
	f->buffer = buffer;
	f->size = size;
	fifo_purge(f);
	return(1);
}

unsigned char fifo_put (fifo_t* f, const unsigned char data)
{
	if(f->buffer == 0) return(0);
	if (f->pwrite==f->pread-1 ||f->pwrite==f->pread-1 +f->size ) return(0);	// return 0 in case of FIFO overflow.

	*(f->pwrite++) = data;	    // copy data byte to buffer
	if(f->pwrite >= f->buffer + f->size) f->pwrite = f->buffer; // start at the begining after reaching the end 
//UDR0=*(f->pwrite-1);
	//f->count++;
	return(1);
}

unsigned char fifo_get (fifo_t* f, unsigned char* pdata)
{
	if(f->buffer == 0) return(0);
	if(f->pread==f->pwrite) return(0);
	*pdata = *(f->pread++);
	if(f->pread >= f->buffer + f->size) f->pread = f->buffer; // start at the begining after reaching the end 

	//f->count--;
	return(1);
}

unsigned char fifo_get_wait (fifo_t* f, unsigned char* pdata)
{
	while (!f->count);

	return fifo_get(f, pdata);
}

void fifo_purge(fifo_t* f)
{
	//if((f == NULL)) return;
	f->count = 0;
	f->pread = f->buffer;
	f->pwrite = f->buffer; 
	return;	
}
