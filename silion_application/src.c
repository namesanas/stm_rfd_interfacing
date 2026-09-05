/*
 * sillion_application.c
 *
 *  Created on: 04-Sept-2026
 *      Author: Identium
 */
#include "stdio.h"

#include "sillion_application.h"

extern void SILION_ProcessRxQueue(void);
extern void VCP_SendTag(const SILION_Tag_t *tag);
extern void SILION_ClearUartFlags(void);

extern volatile uint8_t txComplete;
extern volatile uint32_t silionAsyncPacketCount;
extern volatile uint32_t silionAsyncBadFrameCount;
extern volatile uint32_t g_msTick;
extern SILION_ReaderConfig_t readerConfig;

extern int SILION_WaitForTxComplete(uint32_t timeoutMs);
extern int SILION_WaitForResponse(uint32_t timeoutMs);

extern void VCP_SendString(const char *text);



static Silion_Handle_t *pSilion = NULL;
static SILION_ApplicationState_t appState = SILION_APP_IDLE;

static uint8_t SILION_Application_Transaction(
    uint8_t expectedCommand
)
{
	 int result;

	    /*
	     * --------------------------------------------------------
	     * 1. Wait for TX complete
	     * --------------------------------------------------------
	     */
	    result = SILION_WaitForTxComplete(100U);

	    if(result <= 0)
	    {
	        VCP_SendString(
	            "DEBUG,TX_TIMEOUT\r\n"
	        );

	        return 0U;
	    }

	    /*
	     * --------------------------------------------------------
	     * 2. Wait for SILION response
	     * --------------------------------------------------------
	     */
	    result = SILION_WaitForResponse(1000U);

	    if(result == 0)
	    {
	        VCP_SendString(
	            "DEBUG,RX_TIMEOUT\r\n"
	        );

	        return 0U;
	    }

	    if(result < 0)
	    {
	        if(pSilion->frameError)
	        {
	            VCP_SendString(
	                "DEBUG,FRAME_ERROR\r\n"
	            );
	        }
	        else
	        {
	            VCP_SendString(
	                "DEBUG,RX_UART_ERROR\r\n"
	            );
	        }

	        return 0U;
	    }

	    /*
	     * --------------------------------------------------------
	     * 3. Check returned command
	     * --------------------------------------------------------
	     */
	    if(SILION_GetCommand(pSilion) != expectedCommand)
	    {
	        char debug[64];

	        sprintf(
	            debug,
	            "DEBUG,CMD_MISMATCH,EXP=%02X,GOT=%02X\r\n",
	            expectedCommand,
	            SILION_GetCommand(pSilion)
	        );

	        VCP_SendString(debug);

	        return 0U;
	    }

	    /*
	     * --------------------------------------------------------
	     * 4. Check SILION status
	     * --------------------------------------------------------
	     */
	    if(SILION_GetStatus(pSilion) != SILION_STATUS_SUCCESS)
	    {
	        char debug[64];

	        sprintf(
	            debug,
	            "DEBUG,STATUS=%04X\r\n",
	            SILION_GetStatus(pSilion)
	        );

	        VCP_SendString(debug);

	        return 0U;
	    }

	    return 1U;
}



/*
 * ------------------------------------------------------------
 * Delay while allowing SILION RX processing.
 * ------------------------------------------------------------
 */
static void SILION_Application_DelayMs(uint32_t delayMs)
{
    uint32_t start;

    start = g_msTick;

    while((g_msTick - start) < delayMs)
    {
        SILION_ProcessRxQueue();
    }
}


/*
 * ------------------------------------------------------------
 * Initialize application layer
 * ------------------------------------------------------------
 */
void SILION_Application_Init(
    Silion_Handle_t *pSilionHandle
)
{
    pSilion = pSilionHandle;
    appState = SILION_APP_IDLE;
}


/*
 * ------------------------------------------------------------
 * Configure reader after SILION startup
 *
 * This replaces the large validation sequence that currently
 * lives inside main.c.
 * ------------------------------------------------------------
 */
uint8_t SILION_Application_ConfigureReader(void)
{
    /*
     * --------------------------------------------------------
     * 0x03 GET VERSION
     * --------------------------------------------------------
     */

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();

    txComplete = 0U;

    if(SILION_GetVersion(pSilion) == 0U)
    {
        return 0U;
    }

    if(SILION_Application_Transaction(
           SILION_CMD_GET_VERSION) == 0U)
    {
        return 0U;
    }


    /*
     * --------------------------------------------------------
     * 0x04 BOOT FIRMWARE
     * --------------------------------------------------------
     */

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();

    txComplete = 0U;

    if(SILION_BootFirmware(pSilion) == 0U)
    {
        return 0U;
    }

    if(SILION_Application_Transaction(
           SILION_CMD_BOOT_FIRMWARE) == 0U)
    {
        return 0U;
    }


    /*
     * Module needs time after boot.
     */
    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();

    SILION_Application_DelayMs(500U);


    /*
     * --------------------------------------------------------
     * 0x0C GET RUN PHASE
     * --------------------------------------------------------
     */

    txComplete = 0U;

    if(SILION_GetRunPhase(pSilion) == 0U)
    {
        return 0U;
    }

    if(SILION_Application_Transaction(
           SILION_CMD_GET_RUN_PHASE) == 0U)
    {
        return 0U;
    }

    /*
     * Application firmware expected.
     */
    if(pSilion->rxIndex < 8U)
    {
        return 0U;
    }

    if(pSilion->rxBuffer[5U] != SILION_PROGRAM_APPLICATION)
    {
        return 0U;
    }


    /*
     * --------------------------------------------------------
     * 0x97 SET REGION
     * --------------------------------------------------------
     */

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();

    txComplete = 0U;

    SILION_SetRegion(
        pSilion,
        readerConfig.region
    );

    if(SILION_Application_Transaction(
           SILION_CMD_SET_REGION) == 0U)
    {
        return 0U;
    }


    /*
     * --------------------------------------------------------
     * 0x91 SET INVENTORY ANTENNA
     * --------------------------------------------------------
     */

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();

    txComplete = 0U;

    SILION_SetInventoryAntenna(
        pSilion,
        readerConfig.txAntenna,
        readerConfig.rxAntenna
    );

    /*
     * The existing validated firmware expects
     * SILION_CMD_SET_ANTENNA_PORTS here.
     */
    if(SILION_Application_Transaction(
           SILION_CMD_SET_ANTENNA_PORTS) == 0U)
    {
        return 0U;
    }


    /*
     * --------------------------------------------------------
     * 0x91 SET ANTENNA POWER
     * --------------------------------------------------------
     */

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();

    txComplete = 0U;

    SILION_SetAntennaPower(
        pSilion,
        readerConfig.txAntenna,
        readerConfig.readPower,
        readerConfig.writePower
    );

    /*
     * Keep this response check exactly as already validated
     * in the current working firmware.
     */
    if(SILION_Application_Transaction(
           SILION_CMD_SET_ANTENNA_PORTS) == 0U)
    {
        return 0U;
    }


    /*
     * --------------------------------------------------------
     * 0x93 SET TAG PROTOCOL
     * --------------------------------------------------------
     */

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();

    txComplete = 0U;

    SILION_SetTagProtocol(pSilion);

    if(SILION_Application_Transaction(
           SILION_CMD_SET_TAG_PROTOCOL) == 0U)
    {
        return 0U;
    }


    /*
     * --------------------------------------------------------
     * 0x9B SET GEN2 SESSION
     * --------------------------------------------------------
     */

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();

    txComplete = 0U;

    SILION_SetProtocolSession(
        pSilion,
        readerConfig.session
    );

    if(SILION_Application_Transaction(
           SILION_CMD_SET_PROTOCOL_CONFIG) == 0U)
    {
        return 0U;
    }


    /*
     * --------------------------------------------------------
     * Configuration complete.
     * --------------------------------------------------------
     */

    SILION_ClearFrame(pSilion);

    return 1U;
}

uint8_t SILION_Application_GetVersion(void)
{
    uint32_t bootVersion;
    uint32_t hardwareVersion;
    uint32_t firmwareDate;
    uint32_t firmwareVersion;
    uint32_t supportedProtocol;

    if(pSilion == NULL)
        return 0U;

    if(appState != SILION_APP_IDLE)
        return 0U;

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();
    txComplete = 0U;

    if(SILION_GetVersion(pSilion) == 0U)
        return 0U;

    if(SILION_Application_Transaction(
           SILION_CMD_GET_VERSION) == 0U)
    {
        return 0U;
    }

    if(pSilion->rxIndex < 25U)
        return 0U;

    bootVersion =
        ((uint32_t)pSilion->rxBuffer[5U] << 24) |
        ((uint32_t)pSilion->rxBuffer[6U] << 16) |
        ((uint32_t)pSilion->rxBuffer[7U] << 8)  |
        ((uint32_t)pSilion->rxBuffer[8U]);

    hardwareVersion =
        ((uint32_t)pSilion->rxBuffer[9U] << 24) |
        ((uint32_t)pSilion->rxBuffer[10U] << 16) |
        ((uint32_t)pSilion->rxBuffer[11U] << 8) |
        ((uint32_t)pSilion->rxBuffer[12U]);

    firmwareDate =
        ((uint32_t)pSilion->rxBuffer[13U] << 24) |
        ((uint32_t)pSilion->rxBuffer[14U] << 16) |
        ((uint32_t)pSilion->rxBuffer[15U] << 8) |
        ((uint32_t)pSilion->rxBuffer[16U]);

    firmwareVersion =
        ((uint32_t)pSilion->rxBuffer[17U] << 24) |
        ((uint32_t)pSilion->rxBuffer[18U] << 16) |
        ((uint32_t)pSilion->rxBuffer[19U] << 8) |
        ((uint32_t)pSilion->rxBuffer[20U]);

    supportedProtocol =
        ((uint32_t)pSilion->rxBuffer[21U] << 24) |
        ((uint32_t)pSilion->rxBuffer[22U] << 16) |
        ((uint32_t)pSilion->rxBuffer[23U] << 8) |
        ((uint32_t)pSilion->rxBuffer[24U]);

    {
        char response[180];

        sprintf(
            response,
            "VERSION,BTVER=%08lX,HWVER=%08lX,FWDATE=%08lX,FWVER=%08lX,PROTO=%08lX\r\n",
            (unsigned long)bootVersion,
            (unsigned long)hardwareVersion,
            (unsigned long)firmwareDate,
            (unsigned long)firmwareVersion,
            (unsigned long)supportedProtocol
        );

        VCP_SendString(response);
    }

    SILION_ClearFrame(pSilion);

    return 1U;
}

uint8_t SILION_Application_GetSerial(void)
{
    uint32_t year;
    char response[80];
    uint16_t pos = 0U;
    uint8_t i;

    if(pSilion == NULL)
        return 0U;

    if(appState != SILION_APP_IDLE)
        return 0U;

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();
    txComplete = 0U;

    if(SILION_GetSerialNumber(pSilion) == 0U)
        return 0U;

    if(SILION_Application_Transaction(
           SILION_CMD_GET_SERIAL_NUMBER) == 0U)
    {
        return 0U;
    }

    if(pSilion->rxIndex < 17U)
        return 0U;

    year =
        ((uint32_t)pSilion->rxBuffer[5U] << 24) |
        ((uint32_t)pSilion->rxBuffer[6U] << 16) |
        ((uint32_t)pSilion->rxBuffer[7U] << 8) |
        ((uint32_t)pSilion->rxBuffer[8U]);

    pos += sprintf(
        &response[pos],
        "SERIAL,YEAR=%lu,SN=",
        (unsigned long)year
    );

    for(i = 0U; i < 8U; i++)
    {
        pos += sprintf(
            &response[pos],
            "%02X",
            pSilion->rxBuffer[9U + i]
        );
    }

    pos += sprintf(
        &response[pos],
        "\r\n"
    );

    VCP_SendString(response);

    SILION_ClearFrame(pSilion);

    return 1U;
}

uint8_t SILION_Application_GetTemperature(void)
{
    char response[40];

    if(pSilion == NULL)
        return 0U;

    if(appState != SILION_APP_IDLE)
        return 0U;

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();
    txComplete = 0U;

    if(SILION_GetTemperature(pSilion) == 0U)
        return 0U;

    if(SILION_Application_Transaction(
           SILION_CMD_GET_TEMPERATURE) == 0U)
    {
        return 0U;
    }

    if(pSilion->rxIndex < 8U)
        return 0U;

    sprintf(
        response,
        "TEMP,%d\r\n",
        (int8_t)pSilion->rxBuffer[5U]
    );

    VCP_SendString(response);

    SILION_ClearFrame(pSilion);

    return 1U;
}

uint8_t SILION_Application_GetRegion(void)
{
    char response[40];

    if(pSilion == NULL)
        return 0U;

    if(appState != SILION_APP_IDLE)
        return 0U;

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();
    txComplete = 0U;

    if(SILION_GetCurrentRegion(pSilion) == 0U)
        return 0U;

    if(SILION_Application_Transaction(
           SILION_CMD_GET_CURRENT_REGION) == 0U)
    {
        return 0U;
    }

    if(pSilion->rxIndex < 8U)
        return 0U;

    sprintf(
        response,
        "REGION,%02X\r\n",
        pSilion->rxBuffer[5U]
    );

    VCP_SendString(response);

    SILION_ClearFrame(pSilion);

    return 1U;
}

uint8_t SILION_Application_GetAntenna(void)
{
    char response[50];

    if(pSilion == NULL)
        return 0U;

    if(appState != SILION_APP_IDLE)
        return 0U;

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();
    txComplete = 0U;

    if(SILION_GetAntennaPorts(
           pSilion,
           SILION_ANTENNA_OPTION_INVENTORY) == 0U)
    {
        return 0U;
    }

    if(SILION_Application_Transaction(
           SILION_CMD_GET_ANTENNA_PORTS) == 0U)
    {
        return 0U;
    }

    if(pSilion->rxIndex < 10U)
        return 0U;

    sprintf(
        response,
        "ANTENNA,TX=%u,RX=%u\r\n",
        pSilion->rxBuffer[6U],
        pSilion->rxBuffer[7U]
    );

    VCP_SendString(response);

    SILION_ClearFrame(pSilion);

    return 1U;
}

uint8_t SILION_Application_GetPower(void)
{
    uint16_t readPower;
    uint16_t writePower;
    char response[70];

    if(pSilion == NULL)
        return 0U;

    if(appState != SILION_APP_IDLE)
        return 0U;

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();
    txComplete = 0U;

    if(SILION_GetAntennaPorts(
           pSilion,
           SILION_ANTENNA_OPTION_POWER) == 0U)
    {
        return 0U;
    }

    if(SILION_Application_Transaction(
           SILION_CMD_GET_ANTENNA_PORTS) == 0U)
    {
        return 0U;
    }

    if(pSilion->rxIndex < 12U)
        return 0U;

    readPower =
        ((uint16_t)pSilion->rxBuffer[7U] << 8) |
        pSilion->rxBuffer[8U];

    writePower =
        ((uint16_t)pSilion->rxBuffer[9U] << 8) |
        pSilion->rxBuffer[10U];

    sprintf(
        response,
        "POWER,ANT=%u,READ=%u,WRITE=%u\r\n",
        pSilion->rxBuffer[6U],
        readPower,
        writePower
    );

    VCP_SendString(response);

    SILION_ClearFrame(pSilion);

    return 1U;
}

uint8_t SILION_Application_GetProtocol(void)
{
    char response[40];

    if(pSilion == NULL)
        return 0U;

    if(appState != SILION_APP_IDLE)
        return 0U;

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();
    txComplete = 0U;

    if(SILION_GetTagProtocol(pSilion) == 0U)
        return 0U;

    if(SILION_Application_Transaction(
           SILION_CMD_GET_TAG_PROTOCOL) == 0U)
    {
        return 0U;
    }

    if(pSilion->rxIndex < 9U)
        return 0U;

    sprintf(
        response,
        "PROTOCOL,%02X%02X\r\n",
        pSilion->rxBuffer[5U],
        pSilion->rxBuffer[6U]
    );

    VCP_SendString(response);

    SILION_ClearFrame(pSilion);

    return 1U;
}

uint8_t SILION_Application_GetSession(void)
{
    char response[40];

    if(pSilion == NULL)
        return 0U;

    if(appState != SILION_APP_IDLE)
        return 0U;

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();
    txComplete = 0U;

    if(SILION_GetProtocolConfiguration(
           pSilion,
           SILION_PROTOCOL_GEN2,
           SILION_PROTOCOL_PARAM_SESSION) == 0U)
    {
        return 0U;
    }

    if(SILION_Application_Transaction(
           SILION_CMD_GET_PROTOCOL_CONFIG) == 0U)
    {
        return 0U;
    }

    if(pSilion->rxIndex < 9U)
        return 0U;

    sprintf(
        response,
        "SESSION,%u\r\n",
        pSilion->rxBuffer[7U]
    );

    VCP_SendString(response);

    SILION_ClearFrame(pSilion);

    return 1U;
}

uint8_t SILION_Application_GetFrequency(void)
{
    char response[420];
    uint16_t pos = 0U;
    uint16_t index;
    uint16_t count;
    uint32_t frequency;

    if(pSilion == NULL)
        return 0U;

    if(appState != SILION_APP_IDLE)
        return 0U;

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();
    txComplete = 0U;

    if(SILION_GetFrequencyHopping(pSilion) == 0U)
        return 0U;

    if(SILION_Application_Transaction(
           SILION_CMD_GET_FREQUENCY_HOPPING) == 0U)
    {
        return 0U;
    }

    count = pSilion->expectedLength / 4U;

    pos += sprintf(
        &response[pos],
        "FREQ"
    );

    for(index = 0U; index < count; index++)
    {
        uint16_t offset = (uint16_t)(5U + (index * 4U));

        frequency =
            ((uint32_t)pSilion->rxBuffer[offset] << 24) |
            ((uint32_t)pSilion->rxBuffer[offset + 1U] << 16) |
            ((uint32_t)pSilion->rxBuffer[offset + 2U] << 8) |
            ((uint32_t)pSilion->rxBuffer[offset + 3U]);

        pos += sprintf(
            &response[pos],
            ",%lu",
            (unsigned long)frequency
        );
    }

    pos += sprintf(
        &response[pos],
        "\r\n"
    );

    VCP_SendString(response);

    SILION_ClearFrame(pSilion);

    return 1U;
}

uint8_t SILION_Application_GetRegions(void)
{
    char response[180];
    uint16_t pos = 0U;
    uint16_t i;

    if(pSilion == NULL)
        return 0U;

    if(appState != SILION_APP_IDLE)
        return 0U;

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();
    txComplete = 0U;

    if(SILION_GetAvailableRegions(pSilion) == 0U)
        return 0U;

    if(SILION_Application_Transaction(
           SILION_CMD_GET_AVAILABLE_REGIONS) == 0U)
    {
        return 0U;
    }

    pos += sprintf(
        &response[pos],
        "REGIONS"
    );

    for(i = 0U; i < pSilion->expectedLength; i++)
    {
        pos += sprintf(
            &response[pos],
            ",%02X",
            pSilion->rxBuffer[5U + i]
        );
    }

    pos += sprintf(
        &response[pos],
        "\r\n"
    );

    VCP_SendString(response);

    SILION_ClearFrame(pSilion);

    return 1U;
}

/********************* SETS ************************/

uint8_t SILION_Application_SetPower(
    uint16_t readPower,
    uint16_t writePower
)
{
    if(pSilion == NULL)
        return 0U;

    if(appState != SILION_APP_IDLE)
        return 0U;

    if(readPower > 5000U || writePower > 5000U)
        return 0U;

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();
    txComplete = 0U;

    if(SILION_SetAntennaPower(
           pSilion,
           readerConfig.txAntenna,
           readPower,
           writePower) == 0U)
    {
        return 0U;
    }

    if(SILION_Application_Transaction(
           SILION_CMD_SET_ANTENNA_PORTS) == 0U)
    {
        return 0U;
    }

    readerConfig.readPower = readPower;
    readerConfig.writePower = writePower;

    SILION_ClearFrame(pSilion);

    return 1U;
}

uint8_t SILION_Application_SetAntenna(
    uint8_t txAntenna,
    uint8_t rxAntenna
)
{
    if(pSilion == NULL)
        return 0U;

    if(appState != SILION_APP_IDLE)
        return 0U;

    if(txAntenna == 0U || rxAntenna == 0U)
        return 0U;

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();
    txComplete = 0U;

    if(SILION_SetInventoryAntenna(
           pSilion,
           txAntenna,
           rxAntenna) == 0U)
    {
        return 0U;
    }

    if(SILION_Application_Transaction(
           SILION_CMD_SET_ANTENNA_PORTS) == 0U)
    {
        return 0U;
    }

    readerConfig.txAntenna = txAntenna;
    readerConfig.rxAntenna = rxAntenna;

    SILION_ClearFrame(pSilion);

    return 1U;
}

uint8_t SILION_Application_SetRegion(uint8_t region)
{
    if(pSilion == NULL)
        return 0U;

    if(appState != SILION_APP_IDLE)
        return 0U;

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();
    txComplete = 0U;

    if(SILION_SetRegion(
           pSilion,
           region) == 0U)
    {
        return 0U;
    }

    if(SILION_Application_Transaction(
           SILION_CMD_SET_REGION) == 0U)
    {
        return 0U;
    }

    readerConfig.region = region;

    SILION_ClearFrame(pSilion);

    return 1U;
}

uint8_t SILION_Application_SetSession(uint8_t session)
{
    if(pSilion == NULL)
        return 0U;

    if(appState != SILION_APP_IDLE)
        return 0U;

    if(session > 3U)
        return 0U;

    SILION_ClearFrame(pSilion);
    SILION_ClearUartFlags();
    txComplete = 0U;

    if(SILION_SetProtocolSession(
           pSilion,
           session) == 0U)
    {
        return 0U;
    }

    if(SILION_Application_Transaction(
           SILION_CMD_SET_PROTOCOL_CONFIG) == 0U)
    {
        return 0U;
    }

    readerConfig.session = session;

    SILION_ClearFrame(pSilion);

    return 1U;
}

/*
 * ------------------------------------------------------------
 * Application task
 * ------------------------------------------------------------
 */
void SILION_Application_Task(void)
{
    SILION_Tag_t asyncTag;

    SILION_ProcessRxQueue();

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


/*
 * ------------------------------------------------------------
 * START INVENTORY
 * ------------------------------------------------------------
 */
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


/*
 * ------------------------------------------------------------
 * STOP INVENTORY
 * ------------------------------------------------------------
 */
void SILION_Application_StopInventory(void)
{
    if(appState != SILION_APP_INVENTORY)
    {
        return;
    }

    /*
     * Stop asynchronous inventory.
     */
    SILION_StopAsyncInventory(pSilion);

    /*
     * Make sure the STOP command has actually
     * finished transmitting before allowing another
     * host command to start.
     */
    SILION_WaitForTxComplete(100U);

    /*
     * Now it is safe to return to IDLE.
     */
    appState = SILION_APP_IDLE;
}


/*
 * ------------------------------------------------------------
 * GET APPLICATION STATE
 * ------------------------------------------------------------
 */
SILION_ApplicationState_t SILION_Application_GetState(void)
{
    return appState;
}
