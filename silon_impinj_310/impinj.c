/*
 * impinj.h
 *
 *  Created on: 27-Aug-2026
 *      Author: Identium
 */

#ifndef INC_IMPINJ_H_
#define INC_IMPINJ_H_



#include <stdint.h>
#include "stm32f429xx_driver_uart.h"


/*
 * ============================================================
 * SILION SIM3xxx / IMPINJ E310 DRIVER
 *
 * Protocol reference:
 * PySilion / docs/PROTOCOL.md
 *
 * Transport:
 * STM32F429 USART
 *
 * Frame:
 *
 * Host:
 *   FF | LEN | CMD | DATA | CRC16
 *
 * Reply:
 *   FF | LEN | CMD | STATUS(2) | DATA | CRC16
 *
 * ============================================================
 */


/*
 * ============================================================
 * PROTOCOL CONSTANTS
 * ============================================================
 */

#define SILION_FRAME_HEADER          0xFFU

#define SILION_MAX_FRAME_SIZE        255U

#define SILION_STATUS_SUCCESS        0x0000U


/*
 * ============================================================
 * COMMAND CODES
 * ============================================================
 *
 * We start with only the safe identification command.
 * More commands will be added as we implement them.
 * ============================================================
 */

#define SILION_CMD_GET_VERSION       0x03U
#define SILION_CMD_BOOT_FIRMWARE     0x04U
#define SILION_CMD_GET_RUN_PHASE     0x0CU
#define SILION_CMD_GET_SERIAL_NUMBER 0x10U

#define SILION_CMD_SINGLE_INVENTORY  0x21U
#define SILION_CMD_SYNC_INVENTORY    0x22U
#define SILION_CMD_GET_TAG_BUFFER    0x29U

#define SILION_CMD_ASYNC_INVENTORY   0xAAU
#define SILION_ASYNC_START           0x48U
#define SILION_ASYNC_STOP            0x49U


#define SILION_PROGRAM_BOOTLOADER    0x11U
#define SILION_PROGRAM_APPLICATION   0x12U

#define SILION_CMD_SET_REGION       0x97U
#define SILION_REGION_FULL_BAND     0xFFU

#define SILION_CMD_SET_ANTENNA_PORTS   0x91U

#define SILION_ANTENNA_OPTION_ACCESS   0x00U
#define SILION_ANTENNA_OPTION_INVENTORY 0x02U
#define SILION_ANTENNA_OPTION_POWER    0x03U

#define SILION_CMD_SET_PROTOCOL_CONFIG    0x9BU
#define SILION_CMD_SET_READER_CONFIG      0x9AU
#define SILION_CMD_SET_FREQUENCY_HOPPING   0x95U

#define SILION_CMD_SET_GPO                0x96U
#define SILION_GPO_STATUS_LOW             0x00U
#define SILION_GPO_STATUS_HIGH            0x01U

#define SILION_PROTOCOL_GEN2             0x05U
#define SILION_PROTOCOL_PARAM_SESSION    0x00U
#define SILION_CMD_SET_TAG_PROTOCOL    0x93U
#define SILION_TAG_PROTOCOL_GEN2       0x0005U

#define SILION_SESSION_0                 0x00U
#define SILION_SESSION_1                 0x01U

#define SILION_CMD_SINGLE_TAG_INVENTORY   0x21U

#define SILION_INVENTORY_OPTION_NONE      0x00U
#define SILION_CMD_SYNC_INVENTORY    	  0x22U

#define SILION_CMD_GET_TAG_BUFFER     0x29U

#define SILION_TAG_BUFFER_OPTION_NEW  0x00U
#define SILION_TAG_BUFFER_OPTION_REFETCH 0x01U

#define SILION_TAG_METADATA_NONE   0x0000U
#define SILION_TAG_METADATA_ALL    0x00BFU

#define SILION_CMD_ASYNC_INVENTORY  0xAAU

#define SILION_ASYNC_TAG_QUEUE_SIZE 32U

#define SILION_CMD_GET_TAG_PROTOCOL       0x63U
#define SILION_CMD_GET_CURRENT_REGION     0x67U
#define SILION_CMD_GET_TEMPERATURE        0x72U
#define SILION_CMD_GET_GPI               0x66U

#define SILION_CMD_GET_ANTENNA_PORTS    0x61U
#define SILION_CMD_GET_PROTOCOL_CONFIG    0x6BU

#define SILION_CMD_GET_FREQUENCY_HOPPING   0x65U
#define SILION_CMD_GET_AVAILABLE_REGIONS   0x71U

#define SILION_CMD_GET_READER_CONFIGURATION   0x6AU

#define SILION_PROTOCOL_PARAM_TARGET    0x01U
#define SILION_PROTOCOL_PARAM_MILLER    0x02U
#define SILION_PROTOCOL_PARAM_Q        0x12U

#define SILION_READER_CONFIG_KEY_ANTENNA_IDENTITY   0x00U
#define SILION_READER_CONFIG_KEY_MAX_RSSI            0x06U
#define SILION_READER_CONFIG_KEY_BANK_IDENTITY       0x08U



/*
 * ============================================================
 * RECEIVE STATES
 * ============================================================
 */

typedef enum
{
    SILION_RX_WAIT_HEADER = 0,
    SILION_RX_WAIT_LENGTH,
    SILION_RX_WAIT_COMMAND,
    SILION_RX_WAIT_STATUS_MSB,
    SILION_RX_WAIT_STATUS_LSB,
    SILION_RX_WAIT_DATA,
    SILION_RX_WAIT_CRC_MSB,
    SILION_RX_WAIT_CRC_LSB

} Silion_RxState_t;


/*
 * ============================================================
 * APPLICATION EVENTS
 * ============================================================
 */

#define SILION_EVENT_FRAME_READY     1U
#define SILION_EVENT_FRAME_ERROR     2U


/*
 * ============================================================
 * SILION HANDLE
 * ============================================================
 */

typedef struct
{
    uint8_t  readCount;
    int8_t   rssi;
    uint8_t  antenna;

    uint32_t frequencyKHz;
    uint32_t timestampMs;

    uint16_t epcLengthBits;

    uint16_t pcWord;

    uint8_t  epc[64];
    uint16_t epcLengthBytes;

    uint16_t tagCrc;

} SILION_Tag_t;

typedef struct
{
    SILION_Tag_t tags[SILION_ASYNC_TAG_QUEUE_SIZE];

    volatile uint8_t head;
    volatile uint8_t tail;
    volatile uint8_t count;

    uint32_t overflow;

} SILION_AsyncTagQueue_t;




typedef struct
{
    USART_Handle_t *pUSARTHandle;


    /*
     * --------------------------------------------------------
     * RX frame
     * --------------------------------------------------------
     */

    uint8_t rxBuffer[SILION_MAX_FRAME_SIZE];

    uint16_t rxIndex;

    uint8_t expectedLength;


    /*
     * --------------------------------------------------------
     * Parsed reply information
     * --------------------------------------------------------
     */

    uint8_t command;

    uint16_t status;


    /*
     * --------------------------------------------------------
     * Frame state
     * --------------------------------------------------------
     */

    Silion_RxState_t rxState;

    uint8_t frameReady;

    uint8_t frameError;

    uint32_t asyncPacketCount;
    uint32_t asyncBadPacketCount;
    SILION_AsyncTagQueue_t asyncTagQueue;


} Silion_Handle_t;

/*
 * ============================================================
 * READER CONFIGURATION
 * ============================================================
 */

typedef struct
{
    uint8_t  region;

    uint8_t  txAntenna;
    uint8_t  rxAntenna;

    uint16_t readPower;
    uint16_t writePower;

    uint8_t  tagProtocol;
    uint8_t  session;

} SILION_ReaderConfig_t;

/*
 * ============================================================
 * CRC
 * ============================================================
 */

uint16_t SILION_CalculateCRC(
        const uint8_t *pData,
        uint16_t length);


/*
 * ============================================================
 * FRAME BUILDING
 * ============================================================
 */

uint16_t SILION_BuildCommandFrame(
        uint8_t command,
        const uint8_t *pData,
        uint8_t dataLength,
        uint8_t *pFrame);


/*
 * ============================================================
 * INITIALIZATION
 * ============================================================
 */

void SILION_Init(
        Silion_Handle_t *pSilionHandle,
        USART_Handle_t *pUSARTHandle);


/*
 * ============================================================
 * TRANSMISSION
 * ============================================================
 */

uint8_t SILION_SendFrame(
        Silion_Handle_t *pSilionHandle,
        uint8_t *pFrame,
        uint16_t frameLength);


/*
 * ============================================================
 * GET VERSION
 * ============================================================
 */

uint8_t SILION_GetVersion(
        Silion_Handle_t *pSilionHandle);

uint8_t SILION_BootFirmware(
        Silion_Handle_t *pSilionHandle);

uint8_t SILION_GetRunPhase(
        Silion_Handle_t *pSilionHandle);
uint8_t SILION_GetTagProtocol(
        Silion_Handle_t *pSilionHandle);
uint8_t SILION_GetCurrentRegion(
        Silion_Handle_t *pSilionHandle);
uint8_t SILION_GetTemperature(
        Silion_Handle_t *pSilionHandle);
uint8_t SILION_GetGPI(
        Silion_Handle_t *pSilionHandle);
uint8_t SILION_GetAntennaPorts(
        Silion_Handle_t *pSilionHandle,
        uint8_t option);
uint8_t SILION_GetProtocolConfiguration(
        Silion_Handle_t *pSilionHandle,
        uint8_t protocol,
        uint8_t parameter);
uint8_t SILION_GetFrequencyHopping(
        Silion_Handle_t *pSilionHandle);
uint8_t SILION_GetAvailableRegions(
        Silion_Handle_t *pSilionHandle);
uint8_t SILION_GetSerialNumber(
        Silion_Handle_t *pSilionHandle);
uint8_t SILION_GetReaderConfiguration(
        Silion_Handle_t *pSilionHandle,
        uint8_t key);
uint8_t SILION_SetRegion(
        Silion_Handle_t *pSilionHandle,
        uint8_t region);

//setting apis

uint8_t SILION_SetInventoryAntenna(
        Silion_Handle_t *pSilionHandle,
        uint8_t txAntenna,
        uint8_t rxAntenna);
uint8_t SILION_SetAntennaPower(
        Silion_Handle_t *pSilionHandle,
        uint8_t antenna,
        uint16_t readPower,
        uint16_t writePower);

uint8_t SILION_SetTagProtocol(
        Silion_Handle_t *pSilionHandle);
uint8_t SILION_SetProtocolSession(
        Silion_Handle_t *pSilionHandle,
        uint8_t session);
uint8_t SILION_SetProtocolMiller(
        Silion_Handle_t *pSilionHandle,
        uint8_t miller);
uint8_t SILION_SetProtocolTarget(
        Silion_Handle_t *pSilionHandle,
        uint8_t option,
        uint8_t target);
uint8_t SILION_SetProtocolQ(
        Silion_Handle_t *pSilionHandle,
        uint8_t option,
        uint8_t q);
uint8_t SILION_SetReaderConfiguration(
        Silion_Handle_t *pSilionHandle,
        uint8_t key,
        uint8_t value);
uint8_t SILION_SetGPO(
        Silion_Handle_t *pSilionHandle,
        uint8_t gpoNumber,
        uint8_t status);
uint8_t SILION_SetFrequencyHopping(
        Silion_Handle_t *pSilionHandle,
        const uint32_t *frequencies,
        uint8_t count);

uint8_t SILION_GetGPO(
        Silion_Handle_t *pSilionHandle);

uint8_t SILION_SingleTagInventory(
        Silion_Handle_t *pSilionHandle,
        uint16_t timeoutMs);
uint8_t SILION_SynchronousInventory(
        Silion_Handle_t *pSilionHandle,
        uint16_t timeoutMs);
uint8_t SILION_GetTagBuffer(
        Silion_Handle_t *pSilionHandle,
        uint16_t metadataFlags);

uint8_t SILION_ParseTagBuffer(
        Silion_Handle_t *pSilionHandle,
        SILION_Tag_t *tags,
        uint8_t maxTags,
        uint8_t *tagCount);
uint8_t SILION_AsyncTagQueuePop(
        Silion_Handle_t *pSilionHandle,
        SILION_Tag_t *tag);
uint8_t SILION_AsyncTagQueuePush(
        Silion_Handle_t *pSilionHandle,
        const SILION_Tag_t *tag);
uint8_t SILION_StartAsyncInventory(
        Silion_Handle_t *pSilionHandle);
uint8_t SILION_StopAsyncInventory(
        Silion_Handle_t *pSilionHandle);

/*
 * ============================================================
 * RECEIVE PARSER
 * ============================================================
 */

void SILION_ProcessByte(
        Silion_Handle_t *pSilionHandle,
        uint8_t receivedByte);
uint8_t SILION_ProcessAsyncFrame(
        Silion_Handle_t *pSilionHandle,
        SILION_Tag_t *tag);
uint8_t SILION_ParseAsyncTag(
        Silion_Handle_t *pSilionHandle,
        SILION_Tag_t *tag);

/*
 * ============================================================
 * FRAME STATUS
 * ============================================================
 */

uint8_t SILION_IsFrameReady(
        Silion_Handle_t *pSilionHandle);


uint8_t SILION_ValidateFrame(
        Silion_Handle_t *pSilionHandle);


void SILION_ClearFrame(
        Silion_Handle_t *pSilionHandle);


/*
 * ============================================================
 * RESPONSE ACCESS
 * ============================================================
 */

uint8_t SILION_GetCommand(
        Silion_Handle_t *pSilionHandle);


uint16_t SILION_GetStatus(
        Silion_Handle_t *pSilionHandle);




#endif /* INC_IMPINJ_H_ */
