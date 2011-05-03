#ifndef _SETTINGS_H
#define _SETTINGS_H

typedef enum
{
	PID_KML_LOGGING,
	PID_GPX_LOGGING
} ParamId_t;

void Settings_Init(void);
void Settings_SetDefaultValues(void);
uint8_t Settings_GetParamValue(ParamId_t Pid, uint16_t* pValue);

#endif // _SETTINGS_H


