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

    pSilionHandle->asyncPacketCount = 0U;
    pSilionHandle->asyncBadPacketCount = 0U;
    pSilionHandle->asyncTagQueue.head = 0U;
    pSilionHandle->asyncTagQueue.tail = 0U;
    pSilionHandle->asyncTagQueue.count = 0U;
    pSilionHandle->asyncTagQueue.overflow = 0U;
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
   static uint8_t frame[8];

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
 * BOOT FIRMWARE
 *
 * Protocol:
 *
 *     FF 00 04 1D 0B
 *
 * No data field.
 *
 * This moves the module from the bootloader layer into
 * application firmware.
 * ============================================================
 */
uint8_t SILION_BootFirmware(
        Silion_Handle_t *pSilionHandle)
{
    static uint8_t frame[8];

    uint16_t frameLength;


    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_BOOT_FIRMWARE,
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
 * GET RUN PHASE
 *
 * Protocol:
 *
 *     FF 00 0C 1D 03
 *
 * No data field.
 *
 * Response data:
 *
 *     0x11 = Bootloader
 *     0x12 = Application firmware
 * ============================================================
 */
uint8_t SILION_GetRunPhase(
        Silion_Handle_t *pSilionHandle)
{
    static uint8_t frame[8];

    uint16_t frameLength;


    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_GET_RUN_PHASE,
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

uint8_t SILION_GetTagProtocol(
        Silion_Handle_t *pSilionHandle)
{
	static uint8_t frame[8];
	    uint16_t frameLength;

	    frameLength =
	        SILION_BuildCommandFrame(
	            SILION_CMD_GET_TAG_PROTOCOL,
	            NULL,
	            0U,
	            frame
	        );

	    return SILION_SendFrame(
	        pSilionHandle,
	        frame,
	        frameLength
	    );
}
uint8_t SILION_GetCurrentRegion(
        Silion_Handle_t *pSilionHandle)
{
	  static uint8_t frame[8];
	    uint16_t frameLength;

	    frameLength =
	        SILION_BuildCommandFrame(
	            SILION_CMD_GET_CURRENT_REGION,
	            NULL,
	            0U,
	            frame
	        );

	    return SILION_SendFrame(
	        pSilionHandle,
	        frame,
	        frameLength
	    );
}
uint8_t SILION_GetTemperature(
        Silion_Handle_t *pSilionHandle)
{
   static  uint8_t frame[8];
    uint16_t frameLength;

    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_GET_TEMPERATURE,
            NULL,
            0U,
            frame
        );

    return SILION_SendFrame(
        pSilionHandle,
        frame,
        frameLength
    );
}

uint8_t SILION_GetAntennaPorts(
        Silion_Handle_t *pSilionHandle,
        uint8_t option)
{
    static uint8_t data[1];
    static uint8_t frame[9];

    uint16_t frameLength;

    data[0] = option;

    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_GET_ANTENNA_PORTS,
            data,
            1U,
            frame
        );

    return SILION_SendFrame(
        pSilionHandle,
        frame,
        frameLength
    );
}

uint8_t SILION_GetProtocolConfiguration(
        Silion_Handle_t *pSilionHandle,
        uint8_t protocol,
        uint8_t parameter)
{
    static uint8_t data[2];
    static uint8_t frame[9];

    uint16_t frameLength;

    data[0] = protocol;
    data[1] = parameter;

    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_GET_PROTOCOL_CONFIG,
            data,
            2U,
            frame
        );

    return SILION_SendFrame(
        pSilionHandle,
        frame,
        frameLength
    );
}

uint8_t SILION_GetFrequencyHopping(
        Silion_Handle_t *pSilionHandle)
{
    static uint8_t frame[8];
    uint16_t frameLength;

    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_GET_FREQUENCY_HOPPING,
            NULL,
            0U,
            frame
        );

    return SILION_SendFrame(
        pSilionHandle,
        frame,
        frameLength
    );
}

uint8_t SILION_GetAvailableRegions(
        Silion_Handle_t *pSilionHandle)
{
    static uint8_t frame[8];
    uint16_t frameLength;

    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_GET_AVAILABLE_REGIONS,
            NULL,
            0U,
            frame
        );

    return SILION_SendFrame(
        pSilionHandle,
        frame,
        frameLength
    );
}

uint8_t SILION_GetSerialNumber(
        Silion_Handle_t *pSilionHandle)
{
    static uint8_t data[2];
    static uint8_t frame[9];

    uint16_t frameLength;

    data[0] = 0x00U;
    data[1] = 0x00U;

    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_GET_SERIAL_NUMBER,
            data,
            2U,
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
 * SET CURRENT REGION
 *
 * Region:
 *     0xFF = Full Band
 *
 * Bench/test use only.
 *
 * Frame:
 *
 *     FF 01 97 FF 4B 42
 * ============================================================
 */
uint8_t SILION_SetRegion(
        Silion_Handle_t *pSilionHandle,
        uint8_t region)
{
    static uint8_t frame[8];

    uint16_t frameLength;


    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_SET_REGION,
            &region,
            1,
            frame
        );


    return SILION_SendFrame(
        pSilionHandle,
        frame,
        frameLength
    );
}

uint8_t SILION_SetInventoryAntenna(
        Silion_Handle_t *pSilionHandle,
        uint8_t txAntenna,
        uint8_t rxAntenna)
{
    static uint8_t data[3];
    static uint8_t frame[10];

    uint16_t frameLength;

    data[0] = SILION_ANTENNA_OPTION_INVENTORY;
    data[1] = txAntenna;
    data[2] = rxAntenna;

    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_SET_ANTENNA_PORTS,
            data,
            3,
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
 * SET ANTENNA POWER
 *
 * 0x91 / Option 0x03
 *
 * Power unit:
 *     0.01 dBm
 *
 * Example:
 *     3000 = 30.00 dBm = 0x0BB8
 *
 * Data:
 *
 *     03
 *     antenna
 *     read power MSB
 *     read power LSB
 *     write power MSB
 *     write power LSB
 *
 * ============================================================
 */

uint8_t SILION_SetAntennaPower(
        Silion_Handle_t *pSilionHandle,
        uint8_t antenna,
        uint16_t readPower,
        uint16_t writePower)
{
    static uint8_t data[6];
    static uint8_t frame[13];

    uint16_t frameLength;


    data[0] =
        SILION_ANTENNA_OPTION_POWER;

    data[1] =
        antenna;

    data[2] =
        (uint8_t)(readPower >> 8);

    data[3] =
        (uint8_t)(readPower & 0xFFU);

    data[4] =
        (uint8_t)(writePower >> 8);

    data[5] =
        (uint8_t)(writePower & 0xFFU);


    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_SET_ANTENNA_PORTS,
            data,
            6,
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
 * SET PROTOCOL SESSION
 *
 * 0x9B Set Protocol Configuration
 *
 * Data:
 *
 *     Protocol = 0x05 (GEN2)
 *     Parameter = 0x00 (Session)
 *     Value = session
 *
 * Example from protocol:
 *
 *     FF 03 9B 05 00 01 DC E9
 *
 *     = GEN2, Session 1
 * ============================================================
 */
uint8_t SILION_SetProtocolSession(
        Silion_Handle_t *pSilionHandle,
        uint8_t session)
{
    static uint8_t data[3];
    static uint8_t frame[10];

    uint16_t frameLength;


    data[0] =
        SILION_PROTOCOL_GEN2;

    data[1] =
        SILION_PROTOCOL_PARAM_SESSION;

    data[2] =
        session;


    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_SET_PROTOCOL_CONFIG,
            data,
            3,
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
 * SET TAG PROTOCOL
 *
 * 0x93
 *
 * GEN2 / 18K-6C:
 *
 *     00 05
 *
 * Complete frame:
 *
 *     FF 02 93 00 05 51 7D
 * ============================================================
 */

uint8_t SILION_SetTagProtocol(
        Silion_Handle_t *pSilionHandle)
{
    static uint8_t data[2];
    static uint8_t frame[9];

    uint16_t frameLength;


    data[0] =
        (uint8_t)(SILION_TAG_PROTOCOL_GEN2 >> 8);

    data[1] =
        (uint8_t)(SILION_TAG_PROTOCOL_GEN2 & 0xFFU);


    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_SET_TAG_PROTOCOL,
            data,
            2,
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
 * SINGLE TAG INVENTORY
 *
 * Command:
 *
 *     0x21
 *
 * Option:
 *
 *     0x00 = no filter, EPC only
 *
 * Timeout:
 *
 *     milliseconds
 *
 * Example for 1000 ms:
 *
 *     FF 03 21 03 E8 00 03 2A
 *
 * ============================================================
 */

uint8_t SILION_SingleTagInventory(
        Silion_Handle_t *pSilionHandle,
        uint16_t timeoutMs)
{
    static uint8_t data[3];
    static uint8_t frame[10];

    uint16_t frameLength;


    /*
     * Timeout MSB first.
     */
    data[0] =
        (uint8_t)(timeoutMs >> 8);

    data[1] =
        (uint8_t)(timeoutMs & 0xFFU);


    /*
     * Option:
     *
     * 0x00 = no filter, EPC only.
     */
    data[2] =
        SILION_INVENTORY_OPTION_NONE;


    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_SINGLE_TAG_INVENTORY,
            data,
            3,
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
 * SYNCHRONOUS INVENTORY
 *
 * 0x22
 *
 * Option       = 0x00
 * Search Flags = 0x0000
 * Timeout      = timeoutMs
 *
 * No filter.
 * No embedded command.
 *
 * Response contains the number of tags found.
 * Tags themselves are retrieved with 0x29.
 * ============================================================
 */

uint8_t SILION_SynchronousInventory(
        Silion_Handle_t *pSilionHandle,
        uint16_t timeoutMs)
{
    static uint8_t data[5];
    static uint8_t frame[12];

    uint16_t frameLength;


    /*
     * Option = no filter.
     */
    data[0] = 0x00U;


    /*
     * Search Flags = 0x0000.
     */
    data[1] = 0x00U;
    data[2] = 0x00U;


    /*
     * Timeout, MSB first.
     */
    data[3] =
        (uint8_t)(timeoutMs >> 8);

    data[4] =
        (uint8_t)(timeoutMs & 0xFFU);


    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_SYNC_INVENTORY,
            data,
            5,
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
 * GET TAG BUFFER
 *
 * metadataFlags:
 *
 *   0x0000 = EPC only
 *   0x00BF = read count + RSSI + antenna + frequency +
 *            timestamp + RFU + tag data length
 *
 * option:
 *
 *   0x00 = fetch unread tags
 * ============================================================
 */
uint8_t SILION_GetTagBuffer(
        Silion_Handle_t *pSilionHandle,
        uint16_t metadataFlags)
{
    static uint8_t data[3];
    static uint8_t frame[10];

    uint16_t frameLength;


    data[0] =
        (uint8_t)(metadataFlags >> 8);

    data[1] =
        (uint8_t)(metadataFlags & 0xFFU);

    data[2] =
        SILION_TAG_BUFFER_OPTION_NEW;


    frameLength =
        SILION_BuildCommandFrame(
            SILION_CMD_GET_TAG_BUFFER,
            data,
            3,
            frame
        );


    return SILION_SendFrame(
        pSilionHandle,
        frame,
        frameLength
    );
}

uint8_t SILION_ProcessAsyncFrame(
        Silion_Handle_t *pSilionHandle,
        SILION_Tag_t *tag)
{
    if(
        pSilionHandle == NULL ||
        tag == NULL
    )
    {
        return 0U;
    }

    if(
        pSilionHandle->command
        !=
        SILION_CMD_ASYNC_INVENTORY
    )
    {
        return 0U;
    }

    if(
        pSilionHandle->status
        !=
        SILION_STATUS_SUCCESS
    )
    {
        pSilionHandle->asyncBadPacketCount++;
        return 0U;
    }

    if(
        SILION_ParseAsyncTag(
            pSilionHandle,
            tag
        ) == 0U
    )
    {
        pSilionHandle->asyncBadPacketCount++;
        return 0U;
    }

    if(
        SILION_AsyncTagQueuePush(
            pSilionHandle,
            tag
        ) == 0U
    )
    {
        pSilionHandle->asyncBadPacketCount++;
        return 0U;
    }

    pSilionHandle->asyncPacketCount++;

    return 1U;
}

uint8_t SILION_ParseAsyncTag(
        Silion_Handle_t *pSilionHandle,
        SILION_Tag_t *tag)
{
    uint16_t index;
    uint16_t tagDataLength;
    uint8_t epcTotalBytes;
    uint16_t epcBytes;
    uint16_t metadataFlags;


    if(
        pSilionHandle == NULL ||
        tag == NULL
    )
    {
        return 0U;
    }


    /*
     * --------------------------------------------------------
     * Minimum async data:
     *
     * Metadata Flags = 2 bytes
     * --------------------------------------------------------
     */
    if(
        pSilionHandle->expectedLength < 2U
    )
    {
        return 0U;
    }


    /*
     * --------------------------------------------------------
     * Metadata flags
     *
     * Data begins at rxBuffer[5].
     * --------------------------------------------------------
     */
    metadataFlags =
        ((uint16_t)pSilionHandle->rxBuffer[5] << 8)
        |
        pSilionHandle->rxBuffer[6];


    /*
     * For Stage 2 we expect exactly the metadata requested
     * when starting async inventory.
     *
     * 0x00BF:
     *
     * bit 0  Read Count
     * bit 1  RSSI
     * bit 2  Antenna
     * bit 3  Frequency
     * bit 4  Timestamp
     * bit 5  RFU
     * bit 7  Tag Data Length
     *
     * Protocol ID (bit 6) is not requested.
     */
    if(metadataFlags != SILION_TAG_METADATA_ALL)
    {
        return 0U;
    }


    /*
     * Tag information starts after the metadata flags.
     */
    index = 7U;


    /*
     * --------------------------------------------------------
     * READ COUNT
     * --------------------------------------------------------
     */
    tag->readCount =
        pSilionHandle->rxBuffer[index++];


    /*
     * --------------------------------------------------------
     * RSSI
     * --------------------------------------------------------
     */
    tag->rssi =
        (int8_t)pSilionHandle->rxBuffer[index++];


    /*
     * --------------------------------------------------------
     * ANTENNA
     * --------------------------------------------------------
     */
    tag->antenna =
        pSilionHandle->rxBuffer[index++];


    /*
     * --------------------------------------------------------
     * FREQUENCY
     *
     * 3-byte big-endian value in kHz.
     * --------------------------------------------------------
     */
    tag->frequencyKHz =
        ((uint32_t)pSilionHandle->rxBuffer[index] << 16)
        |
        ((uint32_t)pSilionHandle->rxBuffer[index + 1U] << 8)
        |
        pSilionHandle->rxBuffer[index + 2U];

    index += 3U;


    /*
     * --------------------------------------------------------
     * TIMESTAMP
     *
     * 4-byte big-endian milliseconds.
     * --------------------------------------------------------
     */
    tag->timestampMs =
        ((uint32_t)pSilionHandle->rxBuffer[index] << 24)
        |
        ((uint32_t)pSilionHandle->rxBuffer[index + 1U] << 16)
        |
        ((uint32_t)pSilionHandle->rxBuffer[index + 2U] << 8)
        |
        pSilionHandle->rxBuffer[index + 3U];

    index += 4U;


    /*
     * --------------------------------------------------------
     * RFU
     *
     * 2 bytes.
     * --------------------------------------------------------
     */
    index += 2U;


    /*
     * --------------------------------------------------------
     * TAG DATA LENGTH
     *
     * 2-byte bit length.
     *
     * For our inventory request this should be 0.
     * --------------------------------------------------------
     */
    tagDataLength =
        ((uint16_t)pSilionHandle->rxBuffer[index] << 8)
        |
        pSilionHandle->rxBuffer[index + 1U];

    index += 2U;


    /*
     * Tag-data length must be byte aligned.
     */
    if(
        (tagDataLength % 8U) != 0U
    )
    {
        return 0U;
    }


    /*
     * Skip embedded tag data if present.
     */
    index +=
        (uint16_t)(tagDataLength / 8U);


    /*
     * --------------------------------------------------------
     * EPC LENGTH
     *
     * IMPORTANT:
     *
     * Async 0xAA uses ONE BYTE here.
     *
     * It is the total number of bytes:
     *
     *     PC + EPC + Tag CRC
     *
     * Therefore:
     *
     *     EPC bytes = length - 4
     * --------------------------------------------------------
     */
    epcTotalBytes =
        pSilionHandle->rxBuffer[index++];


    if(epcTotalBytes < 4U)
    {
        return 0U;
    }


    epcBytes =
        (uint16_t)epcTotalBytes - 4U;


    if(
        epcBytes > sizeof(tag->epc)
    )
    {
        return 0U;
    }


    tag->epcLengthBits =
        (uint16_t)epcTotalBytes * 8U;


    tag->epcLengthBytes =
        epcBytes;


    /*
     * --------------------------------------------------------
     * PC WORD
     * --------------------------------------------------------
     */
    tag->pcWord =
        ((uint16_t)pSilionHandle->rxBuffer[index] << 8)
        |
        pSilionHandle->rxBuffer[index + 1U];

    index += 2U;


    /*
     * --------------------------------------------------------
     * Make sure EPC + CRC actually fit.
     * --------------------------------------------------------
     */
    if(
        (index + epcBytes + 2U)
        >
        pSilionHandle->rxIndex
    )
    {
        return 0U;
    }


    /*
     * --------------------------------------------------------
     * EPC
     * --------------------------------------------------------
     */
    for(
        uint16_t i = 0U;
        i < epcBytes;
        i++
    )
    {
        tag->epc[i] =
            pSilionHandle->rxBuffer[index++];
    }


    /*
     * --------------------------------------------------------
     * TAG CRC
     * --------------------------------------------------------
     */
    tag->tagCrc =
        ((uint16_t)pSilionHandle->rxBuffer[index] << 8)
        |
        pSilionHandle->rxBuffer[index + 1U];


    return 1U;
}

uint8_t SILION_AsyncTagQueuePush(
        Silion_Handle_t *pSilionHandle,
        const SILION_Tag_t *tag)
{
    SILION_AsyncTagQueue_t *queue;


    if(
        pSilionHandle == NULL ||
        tag == NULL
    )
    {
        return 0U;
    }


    queue =
        &pSilionHandle->asyncTagQueue;


    /*
     * Queue full.
     */
    if(
        queue->count >=
        SILION_ASYNC_TAG_QUEUE_SIZE
    )
    {
        queue->overflow++;

        return 0U;
    }


    /*
     * Copy complete tag into queue.
     */
    queue->tags[queue->head] =
        *tag;


    /*
     * Advance head.
     */
    queue->head++;

    if(
        queue->head >=
        SILION_ASYNC_TAG_QUEUE_SIZE
    )
    {
        queue->head = 0U;
    }


    queue->count++;


    return 1U;
}

uint8_t SILION_AsyncTagQueuePop(
        Silion_Handle_t *pSilionHandle,
        SILION_Tag_t *tag)
{
    SILION_AsyncTagQueue_t *queue;


    if(
        pSilionHandle == NULL ||
        tag == NULL
    )
    {
        return 0U;
    }


    queue =
        &pSilionHandle->asyncTagQueue;


    /*
     * Nothing available.
     */
    if(queue->count == 0U)
    {
        return 0U;
    }


    /*
     * Copy tag out of queue.
     */
    *tag =
        queue->tags[queue->tail];


    /*
     * Advance tail.
     */
    queue->tail++;

    if(
        queue->tail >=
        SILION_ASYNC_TAG_QUEUE_SIZE
    )
    {
        queue->tail = 0U;
    }


    queue->count--;


    return 1U;
}

uint8_t SILION_ParseTagBuffer(
        Silion_Handle_t *pSilionHandle,
        SILION_Tag_t *tags,
        uint8_t maxTags,
        uint8_t *tagCount)
{
    uint16_t index;
    uint8_t count;
    uint8_t i;

    /*
     * Minimum response data:
     *
     * Metadata Flags = 2
     * Option         = 1
     * Tag Count      = 1
     */
    if(pSilionHandle->expectedLength < 4U)
    {
        return 0U;
    }


    /*
     * 0x29 response:
     *
     * rxBuffer[5] = Metadata Flags MSB
     * rxBuffer[6] = Metadata Flags LSB
     * rxBuffer[7] = Option
     * rxBuffer[8] = Tag Count
     */
    count =
        pSilionHandle->rxBuffer[8];


    if(count > maxTags)
    {
        count = maxTags;
    }


    *tagCount = count;


    /*
     * First tag starts immediately after:
     *
     * FF
     * LEN
     * CMD
     * STATUS MSB
     * STATUS LSB
     * Metadata Flags (2)
     * Option (1)
     * Tag Count (1)
     *
     * Therefore:
     *
     * index = 9
     */
    index = 9U;


    for(i = 0U; i < count; i++)
    {
        uint16_t tagDataLength;
        uint16_t epcLengthBits;
        uint16_t epcTotalBytes;
        uint16_t epcBytes;


        /*
         * ----------------------------------------------------
         * Read Count
         * ----------------------------------------------------
         */
        if(
            index >=
            pSilionHandle->rxIndex
        )
        {
            return 0U;
        }

        tags[i].readCount =
            pSilionHandle->rxBuffer[index++];


        /*
         * ----------------------------------------------------
         * RSSI
         * ----------------------------------------------------
         */
        if(
            index >=
            pSilionHandle->rxIndex
        )
        {
            return 0U;
        }

        tags[i].rssi =
            (int8_t)pSilionHandle->rxBuffer[index++];


        /*
         * ----------------------------------------------------
         * Antenna
         * ----------------------------------------------------
         */
        if(
            index >=
            pSilionHandle->rxIndex
        )
        {
            return 0U;
        }

        tags[i].antenna =
            pSilionHandle->rxBuffer[index++];


        /*
         * ----------------------------------------------------
         * Frequency - 3 bytes, kHz
         * ----------------------------------------------------
         */
        if(
            (index + 2U) >=
            pSilionHandle->rxIndex
        )
        {
            return 0U;
        }

        tags[i].frequencyKHz =
            ((uint32_t)pSilionHandle->rxBuffer[index] << 16)
            |
            ((uint32_t)pSilionHandle->rxBuffer[index + 1U] << 8)
            |
            ((uint32_t)pSilionHandle->rxBuffer[index + 2U]);

        index += 3U;


        /*
         * ----------------------------------------------------
         * Timestamp - 4 bytes, ms
         * ----------------------------------------------------
         */
        if(
            (index + 3U) >=
            pSilionHandle->rxIndex
        )
        {
            return 0U;
        }

        tags[i].timestampMs =
            ((uint32_t)pSilionHandle->rxBuffer[index] << 24)
            |
            ((uint32_t)pSilionHandle->rxBuffer[index + 1U] << 16)
            |
            ((uint32_t)pSilionHandle->rxBuffer[index + 2U] << 8)
            |
            ((uint32_t)pSilionHandle->rxBuffer[index + 3U]);

        index += 4U;


        /*
         * ----------------------------------------------------
         * RFU - 2 bytes
         * ----------------------------------------------------
         */
        if(
            (index + 1U) >=
            pSilionHandle->rxIndex
        )
        {
            return 0U;
        }

        index += 2U;


        /*
         * ----------------------------------------------------
         * Tag Data Length
         * ----------------------------------------------------
         *
         * Our 0x22 command did not request an embedded 0x28,
         * so this should normally be 0.
         */
        if(
            (index + 1U) >=
            pSilionHandle->rxIndex
        )
        {
            return 0U;
        }

        tagDataLength =
            ((uint16_t)pSilionHandle->rxBuffer[index] << 8)
            |
            pSilionHandle->rxBuffer[index + 1U];

        index += 2U;


        /*
         * Skip tag data, if any.
         *
         * Length is in bits.
         */
        if(
            (tagDataLength % 8U) != 0U
        )
        {
            return 0U;
        }

        index +=
            (uint16_t)(tagDataLength / 8U);


        /*
         * ----------------------------------------------------
         * EPC Length
         * ----------------------------------------------------
         *
         * Actual 0x29 hardware response:
         *
         * 2-byte value, in bits.
         */
        if(
            (index + 1U) >=
            pSilionHandle->rxIndex
        )
        {
            return 0U;
        }

        epcLengthBits =
            ((uint16_t)pSilionHandle->rxBuffer[index] << 8)
            |
            pSilionHandle->rxBuffer[index + 1U];

        index += 2U;


        tags[i].epcLengthBits =
            epcLengthBits;


        /*
         * ----------------------------------------------------
         * PC Word
         * ----------------------------------------------------
         */
        if(
            (index + 1U) >=
            pSilionHandle->rxIndex
        )
        {
            return 0U;
        }

        tags[i].pcWord =
            ((uint16_t)pSilionHandle->rxBuffer[index] << 8)
            |
            pSilionHandle->rxBuffer[index + 1U];

        index += 2U;


        /*
         * ----------------------------------------------------
         * EPC length
         *
         * EPC Length includes:
         *
         *     PC  = 2 bytes
         *     EPC = N bytes
         *     CRC = 2 bytes
         */
        if(
            (epcLengthBits % 8U) != 0U
        )
        {
            return 0U;
        }

        epcTotalBytes =
            (uint16_t)(epcLengthBits / 8U);


        if(epcTotalBytes < 4U)
        {
            return 0U;
        }


        epcBytes =
            epcTotalBytes - 4U;


        if(epcBytes > sizeof(tags[i].epc))
        {
            return 0U;
        }


        if(
            (index + epcBytes + 1U) >=
            pSilionHandle->rxIndex
        )
        {
            return 0U;
        }


        tags[i].epcLengthBytes =
            epcBytes;


        /*
         * ----------------------------------------------------
         * EPC
         * ----------------------------------------------------
         */
        for(
            uint16_t n = 0U;
            n < epcBytes;
            n++
        )
        {
            tags[i].epc[n] =
                pSilionHandle->rxBuffer[index++];
        }


        /*
         * ----------------------------------------------------
         * Tag CRC
         * ----------------------------------------------------
         */
        tags[i].tagCrc =
            ((uint16_t)pSilionHandle->rxBuffer[index] << 8)
            |
            pSilionHandle->rxBuffer[index + 1U];

        index += 2U;
    }


    return 1U;
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
	SILION_Tag_t asyncTag;

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

            if(
                SILION_ValidateFrame(
                    pSilionHandle
                )
            )
            {
                /*
                 * --------------------------------------------------------
                 * ASYNCHRONOUS INVENTORY FRAME
                 * --------------------------------------------------------
                 *
                 * 0xAA frames are unsolicited and may arrive continuously.
                 *
                 * Handle them immediately instead of treating them like
                 * a normal request/reply transaction.
                 */
                if(
                    pSilionHandle->command
                    ==
                    SILION_CMD_ASYNC_INVENTORY
                )
                {
                	 SILION_ProcessAsyncFrame(
                	        pSilionHandle,
                	        &asyncTag);
                }
                else
                {
                    /*
                     * Normal request/reply frame.
                     */
                    pSilionHandle->frameReady = 1;
                }
            }
            else
            {
                pSilionHandle->frameError = 1;

                /*
                 * If this was an async packet, count it.
                 */
                if(
                    pSilionHandle->command
                    ==
                    SILION_CMD_ASYNC_INVENTORY
                )
                {
                    pSilionHandle->asyncBadPacketCount++;
                }
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
