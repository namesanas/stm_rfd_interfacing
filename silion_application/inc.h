/*
 * sillion_application.h
 *
 *  Created on: 04-Sept-2026
 *      Author: Identium
 */

#ifndef INC_SILLION_APPLICATION_H_
#define INC_SILLION_APPLICATION_H_

#include "impinj.h"

typedef enum
{
    SILION_APP_IDLE = 0U,
    SILION_APP_INVENTORY
} SILION_ApplicationState_t;

uint8_t SILION_Application_StartInventory(void);
void SILION_Application_StopInventory(void);
SILION_ApplicationState_t SILION_Application_GetState(void);
void SILION_Application_Init(Silion_Handle_t *pSilionHandle);
void SILION_Application_Task(void);

#endif /* INC_SILLION_APPLICATION_H_ */
