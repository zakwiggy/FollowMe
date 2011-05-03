#ifndef _SDC_H_
#define _SDC_H_

#include <inttypes.h>

typedef enum
{
  SD_SUCCESS = 0,
  SD_ERROR_NOCARD,
  SD_ERROR_RESET,
  SD_ERROR_INITIALIZE,
  SD_ERROR_BAD_RESPONSE,
  SD_ERROR_BAD_VOLTAGE_RANGE,
  SD_ERROR_NO_SDCARD,
  SD_ERROR_TIMEOUT,
  SD_ERROR_CRC_DATA,
  SD_ERROR_WRITE_DATA,
  SD_ERROR_READ_DATA,
  SD_ERROR_SET_BLOCKLEN,
  SD_ERROR_UNKNOWN
} SD_Result_t;

extern SD_Result_t SDC_Init(void);
extern SD_Result_t SDC_GetSector (uint32_t Addr, uint8_t *pBuffer);
extern SD_Result_t SDC_PutSector (uint32_t Addr, const uint8_t *pBuffer);
extern SD_Result_t SDC_Deinit(void);

#endif


