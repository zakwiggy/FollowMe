#ifndef _FIFO_H_
#define _FIFO_H_


// the fifo object
typedef struct
{
	unsigned char *buffer;				// pointer to start of the ringbuffer
	unsigned int count;				// number of bytes in FIFO
 	unsigned int size;				// buffer size
	unsigned char *pread;				// read pointer
	unsigned char *pwrite;				// write pointer
} fifo_t;

/*
The initialization of the FIFO sets the read/write pointers etc..
The FIFO uses the buffer 'buf' which byte length must 'size'.
Returns 1 on success ans 0 in case of an error.
*/
unsigned char fifo_init (fifo_t* f, unsigned char* buffer, const unsigned int size);

/*
Puts a byte into the FIFO. Returns 1 on success and 0 in case of FIFO overflow.
*/
unsigned char fifo_put (fifo_t* f, const unsigned char data);

/*
Get the next byte from the FIFO. Returns 0 if the FIFO is empty.
*/
unsigned char fifo_get (fifo_t* f, unsigned char* pdata);

/*
Get the next byte out of the FIFO. If the FIFO is empty the function blocks
until the next byte is put into the FIFO.
*/
unsigned char fifo_get_wait (fifo_t* f, unsigned char* pdata);

/*
Purges the FIFO so that it is empty afterwards
*/
void fifo_purge (fifo_t* f);

#endif /* _FIFO_H_ */
