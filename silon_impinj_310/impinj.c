/*
 * impinj.c
 *
 *  Created on: 27-Aug-2026
 *      Author: Identium
 */


#include "impinj.h"
#include <stddef.h>


/*
 * ============================================================
 * SILION CRC-16/CCITT
 *
 * Initial value : 0xFFFF
 * Polynomial    : 0x1021
 *
 * CRC is calculated over:
 *
 *     Length
 *     Command
 *     Status      <- reply only
 *     Data
 *
 * Header 0xFF and CRC itself are excluded.
 *
 * Reference:
 * PySilion docs/PROTOCOL.md
 * ============================================================
 */

static const uint16_t SILION_CRC_TAB[16] =
{
    0x0000,
    0x1021,
    0x2042,
    0x3063,
    0x4084,
    0x50A5,
    0x60C6,
    0x70E7,
    0x8108,
    0x9129,
    0xA14A,
    0xB16B,
    0xC18C,
    0xD1AD,
    0xE1CE,
    0xF1EF
};


uint16_t SILION_CalculateCRC(
        const uint8_t *pData,
        uint16_t length)
{
    uint16_t crc = 0xFFFFU;

    uint16_t i;


    for(i = 0; i < length; i++)
    {
        crc =
            (uint16_t)(
                (crc << 4) |
                (pData[i] >> 4)
            )
            ^
            SILION_CRC_TAB[crc >> 12];


        crc =
            (uint16_t)(
                (crc << 4) |
                (pData[i] & 0x0FU)
            )
            ^
            SILION_CRC_TAB[crc >> 12];
    }


    return crc;
}


/*
 * ============================================================
 * INITIALIZATION
 * ============================================================
 */

void SILION_Init(
        Silion_Handle_t *pSilionHandle,
        USART_Handle_t *pUSARTHandle)
{
    pSilionHandle->pUSARTHandle =pUSARTHandle;


    pSilionHandle->rxIndex = 0;

    pSilionHandle->expectedLength = 0;


    pSilionHandle->command = 0;

    pSilionHandle->status = 0;


    pSilionHandle->rxState = SILION_RX_WAIT_HEADER;


    pSilionHandle->frameReady = 0;

    pSilionHandle->frameError = 0;
}


/*
 * ============================================================
 * BUILD COMMAND FRAME
 *
 * Host format:
 *
 *     FF
 *     LEN
 *     CMD
 *     DATA...
 *     CRC16
 *
 * LEN counts DATA bytes only.
 *
 * Total frame size:
 *
 *     1 + 1 + 1 + DATA + 2
 *     = DATA + 5
 * ============================================================
 */

uint16_t SILION_BuildCommandFrame(
        uint8_t command,
        const uint8_t *pData,
        uint8_t dataLength,
        uint8_t *pFrame)
{
    uint16_t index = 0;

    uint16_t crc;


    /*
     * --------------------------------------------------------
     * Header
     * --------------------------------------------------------
     */

    pFrame[index++] =
        SILION_FRAME_HEADER;


    /*
     * --------------------------------------------------------
     * Data length
     * --------------------------------------------------------
     */

    pFrame[index++] =
        dataLength;


    /*
     * --------------------------------------------------------
     * Command
     * --------------------------------------------------------
     */

    pFrame[index++] =
        command;


    /*
     * --------------------------------------------------------
     * Data
     * --------------------------------------------------------
     */

    for(uint16_t i = 0; i < dataLength; i++)
    {
        pFrame[index++] =
            pData[i];
    }


    /*
     * --------------------------------------------------------
     * CRC
     *
     * CRC starts at Length.
     *
     * Excludes:
     *
     *     Header
     *     CRC itself
     * --------------------------------------------------------
     */

    crc =
        SILION_CalculateCRC(
            &pFrame[1],
            2U + dataLength
        );


    /*
     * CRC is transmitted MSB first.
     */

    pFrame[index++] =
        (uint8_t)(crc >> 8);

    pFrame[index++] =
        (uint8_t)(crc & 0xFFU);


    return index;
}


/*
 * ============================================================
 * SEND FRAME
 * ============================================================
 */

uint8_t SILION_SendFrame(
        Silion_Handle_t *pSilionHandle,
        uint8_t *pFrame,
        uint16_t frameLength)
{
    if(pSilionHandle == NULL)
    {
        return 0;
    }


    if(pFrame == NULL)
    {
        return 0;
    }


    USART_SendDataIT(
        pSilionHandle->pUSARTHandle,
        pFrame,
        frameLength
    );


    return 1;
}


/*
 * ============================================================
 * GET VERSION
 *
 * Protocol:
 *
 *     FF 00 03 1D 0C
 *
 * This is explicitly documented as the ideal first probe.
 * ============================================================
 */

uint8_t SILION_GetVersion(
        Silion_Handle_t *pSilionHandle)
{
    uint8_t frame[8];

    uint16_t frameLength;


    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_GET_VERSION,
            NULL,
            0,
            frame
        );


    return SILION_SendFrame(
        pSilionHandle,
        frame,
        frameLength
    );
}


/*
 * ============================================================
 * RECEIVE BYTE
 *
 * Reply format:
 *
 * FF
 * LEN
 * CMD
 * STATUS MSB
 * STATUS LSB
 * DATA...
 * CRC MSB
 * CRC LSB
 *
 * Reply total length = LEN + 7
 *
 * Because LEN counts DATA only.
 * ============================================================
 */

void SILION_ProcessByte(
        Silion_Handle_t *pSilionHandle,
        uint8_t receivedByte)
{
    switch(pSilionHandle->rxState)
    {
        /*
         * ----------------------------------------------------
         * SEARCH FOR FF
         * ----------------------------------------------------
         */

        case SILION_RX_WAIT_HEADER:

            if(receivedByte ==
               SILION_FRAME_HEADER)
            {
                pSilionHandle->rxBuffer[0] =
                    receivedByte;


                pSilionHandle->rxIndex =
                    1;


                pSilionHandle->frameError =
                    0;


                pSilionHandle->rxState =
                    SILION_RX_WAIT_LENGTH;
            }

            break;


        /*
         * ----------------------------------------------------
         * LENGTH
         * ----------------------------------------------------
         */

        case SILION_RX_WAIT_LENGTH:

            pSilionHandle->rxBuffer[
                pSilionHandle->rxIndex++
            ] =
                receivedByte;


            pSilionHandle->expectedLength =
                receivedByte;


            pSilionHandle->rxState =
                SILION_RX_WAIT_COMMAND;

            break;


        /*
         * ----------------------------------------------------
         * COMMAND
         * ----------------------------------------------------
         */

        case SILION_RX_WAIT_COMMAND:

            pSilionHandle->rxBuffer[
                pSilionHandle->rxIndex++
            ] =
                receivedByte;


            pSilionHandle->command =
                receivedByte;


            pSilionHandle->rxState =
                SILION_RX_WAIT_STATUS_MSB;

            break;


        /*
         * ----------------------------------------------------
         * STATUS MSB
         * ----------------------------------------------------
         */

        case SILION_RX_WAIT_STATUS_MSB:

            pSilionHandle->rxBuffer[
                pSilionHandle->rxIndex++
            ] =
                receivedByte;


            pSilionHandle->status =
                ((uint16_t)receivedByte << 8);


            pSilionHandle->rxState =
                SILION_RX_WAIT_STATUS_LSB;

            break;


        /*
         * ----------------------------------------------------
         * STATUS LSB
         * ----------------------------------------------------
         */

        case SILION_RX_WAIT_STATUS_LSB:

            pSilionHandle->rxBuffer[
                pSilionHandle->rxIndex++
            ] =
                receivedByte;


            pSilionHandle->status |=
                receivedByte;


            /*
             * No data?
             *
             * Go directly to CRC.
             */

            if(pSilionHandle->expectedLength == 0)
            {
                pSilionHandle->rxState =
                    SILION_RX_WAIT_CRC_MSB;
            }
            else
            {
                pSilionHandle->rxState =
                    SILION_RX_WAIT_DATA;
            }

            break;


        /*
         * ----------------------------------------------------
         * DATA
         * ----------------------------------------------------
         */

        case SILION_RX_WAIT_DATA:

            pSilionHandle->rxBuffer[
                pSilionHandle->rxIndex++
            ] =
                receivedByte;


            /*
             * Data begins at index 5.
             *
             * Therefore:
             *
             * index >= 5 + expectedLength
             *
             * means all data bytes received.
             */

            if(pSilionHandle->rxIndex >=
               (5U +
                pSilionHandle->expectedLength))
            {
                pSilionHandle->rxState =
                    SILION_RX_WAIT_CRC_MSB;
            }

            break;


        /*
         * ----------------------------------------------------
         * CRC MSB
         * ----------------------------------------------------
         */

        case SILION_RX_WAIT_CRC_MSB:

            pSilionHandle->rxBuffer[
                pSilionHandle->rxIndex++
            ] =
                receivedByte;


            pSilionHandle->rxState =
                SILION_RX_WAIT_CRC_LSB;

            break;


        /*
         * ----------------------------------------------------
         * CRC LSB
         * ----------------------------------------------------
         */

        case SILION_RX_WAIT_CRC_LSB:

            pSilionHandle->rxBuffer[
                pSilionHandle->rxIndex++
            ] =
                receivedByte;


            /*
             * Full frame received.
             */

            if(SILION_ValidateFrame(
                    pSilionHandle))
            {
                pSilionHandle->frameReady =
                    1;
            }
            else
            {
                pSilionHandle->frameError =
                    1;
            }


            /*
             * Prepare to search for the next frame.
             */

            pSilionHandle->rxState =
                SILION_RX_WAIT_HEADER;

            break;


        default:

            pSilionHandle->rxState =
                SILION_RX_WAIT_HEADER;

            pSilionHandle->rxIndex =
                0;

            break;
    }
}


/*
 * ============================================================
 * VALIDATE FRAME
 *
 * Reply CRC covers:
 *
 *     LEN
 *     CMD
 *     STATUS
 *     DATA
 *
 * which is exactly:
 *
 *     rxBuffer[1]
 *     through
 *     rxBuffer[4 + expectedLength]
 *
 * Number of bytes:
 *
 *     1 + 1 + 2 + DATA
 *     = DATA + 4
 *
 * Received CRC:
 *
 *     index = 5 + DATA
 *     CRC MSB = that index
 *     CRC LSB = index + 1
 * ============================================================
 */

uint8_t SILION_ValidateFrame(
        Silion_Handle_t *pSilionHandle)
{
    uint16_t calculatedCRC;

    uint16_t receivedCRC;

    uint16_t crcLength;


    /*
     * LEN + CMD + STATUS + DATA
     */

    crcLength =
        4U +
        pSilionHandle->expectedLength;


    calculatedCRC =
        SILION_CalculateCRC(
            &pSilionHandle->rxBuffer[1],
            crcLength
        );


    /*
     * CRC starts after:
     *
     * FF
     * LEN
     * CMD
     * STATUS
     * DATA
     */

    uint16_t crcIndex =
        5U +
        pSilionHandle->expectedLength;


    receivedCRC =
        ((uint16_t)pSilionHandle->rxBuffer[crcIndex]
         << 8);


    receivedCRC |=
        pSilionHandle->rxBuffer[
            crcIndex + 1
        ];


    return
        (calculatedCRC == receivedCRC);
}


/*
 * ============================================================
 * FRAME READY
 * ============================================================
 */

uint8_t SILION_IsFrameReady(
        Silion_Handle_t *pSilionHandle)
{
    return pSilionHandle->frameReady;
}


/*
 * ============================================================
 * CLEAR FRAME
 * ============================================================
 */

void SILION_ClearFrame(
        Silion_Handle_t *pSilionHandle)
{
    pSilionHandle->frameReady =
        0;


    pSilionHandle->frameError =
        0;


    pSilionHandle->rxIndex =
        0;


    pSilionHandle->expectedLength =
        0;


    pSilionHandle->command =
        0;


    pSilionHandle->status =
        0;


    pSilionHandle->rxState =
        SILION_RX_WAIT_HEADER;
}


/*
 * ============================================================
 * GET COMMAND
 * ============================================================
 */

uint8_t SILION_GetCommand(
        Silion_Handle_t *pSilionHandle)
{
    return pSilionHandle->command;
}


/*
 * ============================================================
 * GET STATUS
 * ============================================================
 */

uint16_t SILION_GetStatus(
        Silion_Handle_t *pSilionHandle)
{
    return pSilionHandle->status;
}
