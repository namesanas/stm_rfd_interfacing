#ifndef INC_SILLION_APPLICATION_H_
#define INC_SILLION_APPLICATION_H_

#include "impinj.h"

typedef enum
{
    SILION_APP_IDLE = 0U,
    SILION_APP_INVENTORY
} SILION_ApplicationState_t;


/*
 * ------------------------------------------------------------
 * Application initialization / task
 * ------------------------------------------------------------
 */
void SILION_Application_Init(Silion_Handle_t *pSilionHandle);

uint8_t SILION_Application_ConfigureReader(void);

void SILION_Application_Task(void);


/*
 * ------------------------------------------------------------
 * Reader information
 * ------------------------------------------------------------
 */
uint8_t SILION_Application_GetVersion(void);
uint8_t SILION_Application_GetSerial(void);
uint8_t SILION_Application_GetTemperature(void);
uint8_t SILION_Application_GetRegion(void);
uint8_t SILION_Application_GetAntenna(void);
uint8_t SILION_Application_GetPower(void);
uint8_t SILION_Application_GetProtocol(void);
uint8_t SILION_Application_GetSession(void);
uint8_t SILION_Application_GetFrequency(void);
uint8_t SILION_Application_GetRegions(void);


/*
 * ------------------------------------------------------------
 * Reader settings
 * ------------------------------------------------------------
 */
uint8_t SILION_Application_SetPower(
    uint16_t readPower,
    uint16_t writePower
);

uint8_t SILION_Application_SetAntenna(
    uint8_t txAntenna,
    uint8_t rxAntenna
);

uint8_t SILION_Application_SetRegion(
    uint8_t region
);

uint8_t SILION_Application_SetSession(
    uint8_t session
);


/*
 * ------------------------------------------------------------
 * Inventory
 * ------------------------------------------------------------
 */
uint8_t SILION_Application_StartInventory(void);

void SILION_Application_StopInventory(void);

SILION_ApplicationState_t
SILION_Application_GetState(void);

#endif /* INC_SILLION_APPLICATION_H_ */
