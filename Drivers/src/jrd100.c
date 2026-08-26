include "jrd100.h"
#include "stdio.h"


void JRD100_Init(JRD100_Handle_t *pJRD100Handle,
                 USART_Handle_t *pUSARTHandle)
{
    pJRD100Handle->pUSARTHandle = pUSARTHandle;

    pJRD100Handle->rxIndex = 0;
    pJRD100Handle->expectedLength = 0;

    pJRD100Handle->frameReady = 0;
    pJRD100Handle->frameError = 0;

    pJRD100Handle->rxState = JRD100_RX_WAIT_HEADER;
}

uint8_t JRD100_CalculateChecksum(uint8_t *pData,
                                 uint16_t length)
{
    uint16_t sum = 0;
    uint16_t i;

    for(i = 0; i < length; i++)
    {
        sum += pData[i];
    }

    return (uint8_t)(sum & 0xFFU);
}

uint16_t JRD100_BuildFrame(uint8_t type,
                           uint8_t command,
                           uint8_t *pParameter,
                           uint16_t parameterLength,
                           uint8_t *pFrame)
{
    uint16_t index = 0;
    uint16_t i;

    /* Header */
    pFrame[index++] = JRD100_FRAME_HEADER;

    /* Type */
    pFrame[index++] = type;

    /* Command */
    pFrame[index++] = command;

    /* Parameter Length MSB */
    pFrame[index++] = (uint8_t)(parameterLength >> 8);

    /* Parameter Length LSB */
    pFrame[index++] = (uint8_t)(parameterLength & 0xFFU);

    /* Parameters */
    for(i = 0; i < parameterLength; i++)
    {
        pFrame[index++] = pParameter[i];
    }

    /*
     * Checksum covers:
     *
     * Type
     * Command
     * Length MSB
     * Length LSB
     * Parameter bytes
     */

    pFrame[index] =
        JRD100_CalculateChecksum(&pFrame[1], index - 1);

    index++;

    /* End byte */
    pFrame[index++] = JRD100_FRAME_END;

    return index;
}

uint8_t JRD100_SendFrame(JRD100_Handle_t *pJRD100Handle,
                         uint8_t *pFrame,
                         uint16_t frameLength)
{
    if(pJRD100Handle == NULL)
    {
        return 0;
    }

    if(pFrame == NULL)
    {
        return 0;
    }

    USART_SendDataIT(
        pJRD100Handle->pUSARTHandle,
        pFrame,
        frameLength
    );

    return 1;
}

uint8_t JRD100_GetReaderInfo(
    JRD100_Handle_t *pJRD100Handle,
    uint8_t infoType)
{
    uint8_t parameter = infoType;
    static uint8_t frame[16];
    uint16_t frameLength;

    frameLength = JRD100_BuildFrame(
        JRD100_TYPE_COMMAND,
        JRD100_CMD_GET_INFO,
        &parameter,
        1,
        frame
    );

    printf("FRAME LEN = %u\r\n", frameLength);

    for(uint16_t i = 0; i < frameLength; i++)
    {
        printf("%02X ", frame[i]);
    }

    printf("\r\n");

    return JRD100_SendFrame(
        pJRD100Handle,
        frame,
        frameLength
    );
}

void JRD100_ProcessByte(JRD100_Handle_t *pJRD100Handle,
                        uint8_t receivedByte)
{
    switch(pJRD100Handle->rxState)
    {
        case JRD100_RX_WAIT_HEADER:

            if(receivedByte == JRD100_FRAME_HEADER)
            {
                pJRD100Handle->rxBuffer[0] = receivedByte;

                pJRD100Handle->rxIndex = 1;
                pJRD100Handle->frameError = 0;

                pJRD100Handle->rxState = JRD100_RX_WAIT_TYPE;
            }

            break;


        case JRD100_RX_WAIT_TYPE:

            pJRD100Handle->rxBuffer[pJRD100Handle->rxIndex++] =
                receivedByte;

            pJRD100Handle->rxState = JRD100_RX_WAIT_COMMAND;

            break;


        case JRD100_RX_WAIT_COMMAND:

            pJRD100Handle->rxBuffer[pJRD100Handle->rxIndex++] =
                receivedByte;

            pJRD100Handle->rxState = JRD100_RX_WAIT_LENGTH_MSB;

            break;


        case JRD100_RX_WAIT_LENGTH_MSB:

            pJRD100Handle->rxBuffer[pJRD100Handle->rxIndex++] =
                receivedByte;

            pJRD100Handle->expectedLength =
                ((uint16_t)receivedByte << 8);

            pJRD100Handle->rxState = JRD100_RX_WAIT_LENGTH_LSB;

            break;


        case JRD100_RX_WAIT_LENGTH_LSB:

            pJRD100Handle->rxBuffer[pJRD100Handle->rxIndex++] =
                receivedByte;

            pJRD100Handle->expectedLength |= receivedByte;

            if(pJRD100Handle->expectedLength >
               (JRD100_MAX_FRAME_SIZE - 7U))
            {
                pJRD100Handle->frameError = 1;
                pJRD100Handle->rxState =
                    JRD100_RX_WAIT_HEADER;
                pJRD100Handle->rxIndex = 0;

                break;
            }

            if(pJRD100Handle->expectedLength == 0)
            {
                pJRD100Handle->rxState =
                    JRD100_RX_WAIT_CHECKSUM;
            }
            else
            {
                pJRD100Handle->rxState =
                    JRD100_RX_WAIT_PARAMETER;
            }

            break;


        case JRD100_RX_WAIT_PARAMETER:

            pJRD100Handle->rxBuffer[pJRD100Handle->rxIndex++] =
                receivedByte;

            if(pJRD100Handle->rxIndex >=
               (5U + pJRD100Handle->expectedLength))
            {
                pJRD100Handle->rxState =
                    JRD100_RX_WAIT_CHECKSUM;
            }

            break;


        case JRD100_RX_WAIT_CHECKSUM:

            pJRD100Handle->rxBuffer[pJRD100Handle->rxIndex++] =
                receivedByte;

            pJRD100Handle->rxState =
                JRD100_RX_WAIT_END;

            break;


        case JRD100_RX_WAIT_END:

            if(receivedByte == JRD100_FRAME_END)
            {
                pJRD100Handle->rxBuffer[pJRD100Handle->rxIndex++] =
                    receivedByte;

                pJRD100Handle->frameReady = 1;
            }
            else
            {
                pJRD100Handle->frameError = 1;
            }

            pJRD100Handle->rxState =
                JRD100_RX_WAIT_HEADER;

            break;


        default:

            pJRD100Handle->rxState =
                JRD100_RX_WAIT_HEADER;

            pJRD100Handle->rxIndex = 0;

            break;
    }
}

uint8_t JRD100_ValidateFrame(JRD100_Handle_t *pJRD100Handle)
{
    uint8_t calculatedChecksum;
    uint8_t receivedChecksum;

    uint16_t checksumLength;

    /*
     * Checksum starts at Type (index 1)
     * and includes Command, Length and Parameters.
     */

    checksumLength =
        4U + pJRD100Handle->expectedLength;

    calculatedChecksum =
        JRD100_CalculateChecksum(
            &pJRD100Handle->rxBuffer[1],
            checksumLength
        );

    receivedChecksum =
        pJRD100Handle->rxBuffer[
            5U + pJRD100Handle->expectedLength
        ];

    if(calculatedChecksum == receivedChecksum)
    {
        return 1;
    }

    return 0;
}

uint8_t JRD100_IsFrameReady(JRD100_Handle_t *pJRD100Handle)
{
    return pJRD100Handle->frameReady;
}

void JRD100_ClearFrame(JRD100_Handle_t *pJRD100Handle)
{
    pJRD100Handle->frameReady = 0;
    pJRD100Handle->rxIndex = 0;
    pJRD100Handle->expectedLength = 0;
    pJRD100Handle->rxState = JRD100_RX_WAIT_HEADER;
}
