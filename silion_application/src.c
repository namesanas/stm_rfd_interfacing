/*
 * sillion_application.c
 *
 *  Created on: 04-Sept-2026
 *      Author: Identium
 */


#include "sillion_application.h"
extern void SILION_ProcessRxQueue(void);
extern void VCP_SendTag(const SILION_Tag_t *tag);
extern void SILION_ClearUartFlags(void);
extern volatile uint8_t txComplete;
extern volatile uint32_t silionAsyncPacketCount;
extern volatile uint32_t silionAsyncBadFrameCount;
extern int SILION_WaitForTxComplete(uint32_t timeoutMs);


static Silion_Handle_t *pSilion = NULL;

static SILION_ApplicationState_t appState = SILION_APP_IDLE;



void SILION_Application_Init(
        Silion_Handle_t *pSilionHandle)
{
    pSilion = pSilionHandle;
    appState = SILION_APP_IDLE;
}

void SILION_Application_Task(void)
{
    SILION_Tag_t asyncTag;

    /*
     * Process every byte received from the SILION module.
     */
    SILION_ProcessRxQueue();

    /*
     * Send all completed RFID tags to the PC.
     */
    if(
        SILION_AsyncTagQueuePop(
            pSilion,
            &asyncTag
        )
    )
    {
        VCP_SendTag(&asyncTag);
    }
}

uint8_t SILION_Application_StartInventory(void)
{
    if(pSilion == NULL)
    {
        return 0U;
    }

    if(appState != SILION_APP_IDLE)
    {
        return 0U;
    }

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();

    txComplete = 0U;

    silionAsyncPacketCount = 0U;
    silionAsyncBadFrameCount = 0U;

    if(SILION_StartAsyncInventory(pSilion) == 0U)
    {
        return 0U;
    }

    if(SILION_WaitForTxComplete(100U) <= 0)
    {
        return 0U;
    }

    appState = SILION_APP_INVENTORY;

    return 1U;
}



void SILION_Application_StopInventory(void)
{
    if(appState != SILION_APP_INVENTORY)
    {
        return;
    }

    SILION_StopAsyncInventory(pSilion);

    appState = SILION_APP_IDLE;
}

SILION_ApplicationState_t SILION_Application_GetState(void)
{
    return appState;
}




