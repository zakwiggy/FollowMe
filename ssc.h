#ifndef __SSC_H
#define __SSC_H

#include <inttypes.h>

#ifdef USE_FOLLOWME
#define SD_SWITCH   !(PINB & (1<<PINB2))
#endif
#ifdef USE_SDLOGGER
#define SD_SWITCH   !(PINB & (1<<PINB3))
#endif

extern void 	SSC_Init(void);
extern uint8_t	SSC_GetChar(void);
extern void 	SSC_PutChar(uint8_t);
extern void 	SSC_Enable(void);
extern void 	SSC_Disable(void);
extern void		SSC_Deinit(void);

#endif //__SSC_H
