#ifndef INC_JRD100_DRIVER_H_
#define INC_JRD100_DRIVER_H_

#include <stdint.h>
#include "stm32f429xx.h"
#include "stm32f429xx_usart_driver.h"


/*
 * JRD-100 Frame Constants
 */

#define JRD100_FRAME_HEADER        0xBBU
#define JRD100_FRAME_END           0x7EU

#define JRD100_TYPE_COMMAND        0x00U
#define JRD100_TYPE_RESPONSE       0x01U
#define JRD100_TYPE_NOTIFICATION   0x02U


/*
 * JRD-100 Command Codes
 */

#define JRD100_CMD_GET_INFO        0x03U
#define JRD100_CMD_SINGLE_POLL     0x22U
#define JRD100_CMD_MULTI_POLL      0x27U
#define JRD100_CMD_STOP_POLL       0x28U

#define JRD100_CMD_SET_BAUD        0x11U
#define JRD100_CMD_GET_QUERY       0x0DU
#define JRD100_CMD_SET_QUERY       0x0EU
#define JRD100_CMD_SET_REGION      0x07U
#define JRD100_CMD_GET_REGION      0x08U


/*
 * JRD-100 Driver Configuration
 */

#define JRD100_MAX_FRAME_SIZE      256U


/*
 * JRD-100 Driver Handle
 */

typedef struct
{
    USART_Handle_t *pUSARTHandle;

    uint8_t rxBuffer[JRD100_MAX_FRAME_SIZE];

    uint16_t rxIndex;
    uint16_t expectedLength;

    uint8_t frameReady;
    uint8_t frameError;

    JRD100_RXState_t rxState;

} JRD100_Handle_t;

typedef enum
{
    JRD100_RX_WAIT_HEADER = 0,
    JRD100_RX_WAIT_TYPE,
    JRD100_RX_WAIT_COMMAND,
    JRD100_RX_WAIT_LENGTH_MSB,
    JRD100_RX_WAIT_LENGTH_LSB,
    JRD100_RX_WAIT_PARAMETER,
    JRD100_RX_WAIT_CHECKSUM,
    JRD100_RX_WAIT_END

} JRD100_RXState_t;

/*
 * Driver APIs
 */

void JRD100_Init(JRD100_Handle_t *pJRD100Handle,
                 USART_Handle_t *pUSARTHandle);

uint8_t JRD100_CalculateChecksum(uint8_t *pData,
                                 uint16_t length);

uint16_t JRD100_BuildFrame(uint8_t type,
                           uint8_t command,
                           uint8_t *pParameter,
                           uint16_t parameterLength,
                           uint8_t *pFrame);

uint8_t JRD100_SendFrame(JRD100_Handle_t *pJRD100Handle,
                         uint8_t *pFrame,
                         uint16_t frameLength);

uint8_t JRD100_GetReaderInfo(JRD100_Handle_t *pJRD100Handle,
                             uint8_t infoType);

uint8_t JRD100_SinglePolling(JRD100_Handle_t *pJRD100Handle);

void JRD100_ProcessByte(JRD100_Handle_t *pJRD100Handle,uint8_t receivedByte);

uint8_t JRD100_IsFrameReady(JRD100_Handle_t *pJRD100Handle);

void JRD100_ClearFrame(JRD100_Handle_t *pJRD100Handle);

uint8_t JRD100_ValidateFrame(JRD100_Handle_t *pJRD100Handle);

#endif /* INC_JRD100_DRIVER_H_ */
