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

#define SILION_CMD_SINGLE_INVENTORY  0x21U
#define SILION_CMD_SYNC_INVENTORY    0x22U
#define SILION_CMD_GET_TAG_BUFFER    0x29U

#define SILION_CMD_ASYNC_INVENTORY   0xAAU


#define SILION_PROGRAM_BOOTLOADER    0x11U
#define SILION_PROGRAM_APPLICATION   0x12U

#define SILION_CMD_SET_REGION       0x97U
#define SILION_REGION_FULL_BAND     0xFFU

#define SILION_CMD_SET_ANTENNA_PORTS   0x91U

#define SILION_ANTENNA_OPTION_ACCESS   0x00U
#define SILION_ANTENNA_OPTION_INVENTORY 0x02U
#define SILION_ANTENNA_OPTION_POWER    0x03U

#define SILION_CMD_SET_PROTOCOL_CONFIG    0x9BU

#define SILION_PROTOCOL_GEN2             0x05U
#define SILION_PROTOCOL_PARAM_SESSION    0x00U
#define SILION_SESSION_0                 0x00U
#define SILION_SESSION_1                 0x01U

#define SILION_CMD_SINGLE_TAG_INVENTORY   0x21U

#define SILION_INVENTORY_OPTION_NONE      0x00U



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


} Silion_Handle_t;


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
uint8_t SILION_SetRegion(
        Silion_Handle_t *pSilionHandle,
        uint8_t region);
uint8_t SILION_SetInventoryAntenna(
        Silion_Handle_t *pSilionHandle,
        uint8_t txAntenna,
        uint8_t rxAntenna);
uint8_t SILION_SetAntennaPower(
        Silion_Handle_t *pSilionHandle,
        uint8_t antenna,
        uint16_t readPower,
        uint16_t writePower);
uint8_t SILION_SetProtocolSession(
        Silion_Handle_t *pSilionHandle,
        uint8_t session);
uint8_t SILION_SingleTagInventory(
        Silion_Handle_t *pSilionHandle,
        uint16_t timeoutMs);


/*
 * ============================================================
 * RECEIVE PARSER
 * ============================================================
 */

void SILION_ProcessByte(
        Silion_Handle_t *pSilionHandle,
        uint8_t receivedByte);


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
