/*
 * w5500.c
 *
 *  Created on: 15-Sept-2026
 *      Author: Identium
 */


#include "w5500.h"
#include "config.h"

extern void SILION_DelayMs(uint32_t delayMs);


static void W5500_CS_Low(void)
{
    GPIO_WriteToOutputPin(W5500_CS_GPIO_PORT,
                          W5500_CS_PIN,
                          GPIO_PIN_RESET);
}

static void W5500_CS_High(void)
{
    GPIO_WriteToOutputPin(W5500_CS_GPIO_PORT,
                          W5500_CS_PIN,
                          GPIO_PIN_SET);
}

static uint8_t W5500_SPI_Transfer(uint8_t data)
{
    while (!(SPI1->SR & (1U << SPI_SR_TXE)))
    {
    }

    *((volatile uint8_t *)&SPI1->DR) = data;

    while (!(SPI1->SR & (1U << SPI_SR_RXNE)))
    {
    }

    return *((volatile uint8_t *)&SPI1->DR);
}

static void W5500_SPI_WaitUntilNotBusy(void)
{
    while (SPI1->SR & (1U << SPI_SR_BSY))
    {
    }
}

static uint8_t W5500_MakeControlByte(
    uint8_t blockSelect,
    uint8_t write
)
{
    uint8_t control = 0U;

    control |= (uint8_t)((blockSelect & 0x1FU) << 3);

    if (write)
    {
        control |= (1U << 2);
    }

    /*
     * OM[1:0] = 00
     * Variable-length data mode
     */

    return control;
}

static void W5500_WriteAddress(uint16_t address)
{
    W5500_SPI_Transfer(
        (uint8_t)(address >> 8)
    );

    W5500_SPI_Transfer(
        (uint8_t)(address & 0xFFU)
    );
}

uint8_t W5500_ReadRegisters(
    uint16_t address,
    uint8_t blockSelect,
    uint8_t *buffer,
    uint16_t length
)
{
    uint16_t i;

    if ((buffer == NULL) || (length == 0U))
    {
        return 0U;
    }

    W5500_CS_Low();

    W5500_WriteAddress(address);

    W5500_SPI_Transfer(
        W5500_MakeControlByte(
            blockSelect,
            0U
        )
    );

    for (i = 0U; i < length; i++)
    {
        buffer[i] =
            W5500_SPI_Transfer(0x00U);
    }

    W5500_SPI_WaitUntilNotBusy();

    W5500_CS_High();

    return 1U;
}

uint8_t W5500_WriteRegisters(
    uint16_t address,
    uint8_t blockSelect,
    const uint8_t *buffer,
    uint16_t length
)
{
    uint16_t i;

    if ((buffer == NULL) || (length == 0U))
    {
        return 0U;
    }

    W5500_CS_Low();

    W5500_WriteAddress(address);

    W5500_SPI_Transfer(
        W5500_MakeControlByte(
            blockSelect,
            1U
        )
    );

    for (i = 0U; i < length; i++)
    {
        W5500_SPI_Transfer(buffer[i]);
    }

    W5500_SPI_WaitUntilNotBusy();

    W5500_CS_High();

    return 1U;
}

void W5500_Init(void)
{

    W5500_CS_High();

    GPIO_WriteToOutputPin(W5500_RST_GPIO_PORT,
                          W5500_RST_PIN,
                          GPIO_PIN_RESET);

    SILION_DelayMs(2);

    GPIO_WriteToOutputPin(W5500_RST_GPIO_PORT,
                          W5500_RST_PIN,
                          GPIO_PIN_SET);

    SILION_DelayMs(50);
}

void W5500_SetNetworkConfig(void)
{
    static const uint8_t gateway[4] =
    {
        0U, 0U, 0U, 0U
    };

    static const uint8_t subnet[4] =
    {
        255U, 255U, 255U, 0U
    };

    static const uint8_t mac[6] =
    {
    	0x11U,0x22U,0x33U,0x44U,0x55U,0x66U

    };

    static const uint8_t ip[4] =
    {
        192U, 168U, 50U, 231U
    };

    W5500_WriteRegisters(
        W5500_GAR,
        0U,
        gateway,
        4U
    );

    W5500_WriteRegisters(
        W5500_SUBR,
        0U,
        subnet,
        4U
    );

    W5500_WriteRegisters(
        W5500_SHAR,
        0U,
        mac,
        6U
    );

    W5500_WriteRegisters(
        W5500_SIPR,
        0U,
        ip,
        4U
    );
}

uint8_t W5500_ReadVersion(void)
{
    uint8_t version = 0U;

    if (!W5500_ReadRegisters(
            W5500_VERSIONR,
            0U,
            &version,
            1U))
    {
        return 0U;
    }

    return version;
}

uint8_t W5500_Socket0_GetStatus(void)
{
    uint8_t status = W5500_Sn_SR_CLOSED;

    W5500_ReadRegisters(
        W5500_S0_SR,
        W5500_S0_BSB,
        &status,
        1U
    );

    return status;
}


uint8_t W5500_Socket0_Close(void)
{
    uint8_t command;
    uint8_t interruptFlags = 0xFFU;
    uint8_t status;
    uint8_t attempt;

    /* Clear pending socket interrupts. */
    W5500_WriteRegisters(
        W5500_S0_IR,
        W5500_S0_BSB,
        &interruptFlags,
        1U
    );

    /* CLOSE socket. */
    command = W5500_Sn_CR_CLOSE;

    W5500_WriteRegisters(
        W5500_S0_CR,
        W5500_S0_BSB,
        &command,
        1U
    );

    /* Wait until W5500 reports SOCK_CLOSED. */
    for(attempt = 0U; attempt < 20U; attempt++)
    {
        SILION_DelayMs(1U);

        status = W5500_Socket0_GetStatus();

        if(status == W5500_Sn_SR_CLOSED)
        {
            return 1U;
        }
    }

    return 0U;
}

uint8_t W5500_StartTCPServer(uint16_t port)
{
    uint8_t mode = 0x01U;
    uint8_t portData[2];
    uint8_t command;
    uint8_t status = 0U;
    uint8_t attempt;

    portData[0] = (uint8_t)(port >> 8);
    portData[1] = (uint8_t)(port & 0xFFU);

    /* TCP mode */
    W5500_WriteRegisters(
        W5500_S0_MR,
        W5500_S0_BSB,
        &mode,
        1U
    );

    /* Local listening port */
    W5500_WriteRegisters(
        W5500_S0_PORT,
        W5500_S0_BSB,
        portData,
        2U
    );

    /* OPEN */
    command = W5500_Sn_CR_OPEN;

    W5500_WriteRegisters(
        W5500_S0_CR,
        W5500_S0_BSB,
        &command,
        1U
    );

    /* OPEN is asynchronous. Wait for SOCK_INIT. */
    for(attempt = 0U; attempt < 20U; attempt++)
    {
        SILION_DelayMs(1U);

        status = W5500_Socket0_GetStatus();

        if(status == W5500_Sn_SR_INIT)
        {
            break;
        }
    }

    if(status != W5500_Sn_SR_INIT)
    {
        return 0U;
    }

    /* LISTEN */
    command = W5500_Sn_CR_LISTEN;

    W5500_WriteRegisters(
        W5500_S0_CR,
        W5500_S0_BSB,
        &command,
        1U
    );

    /* LISTEN is asynchronous. Wait for SOCK_LISTEN. */
    for(attempt = 0U; attempt < 20U; attempt++)
    {
        SILION_DelayMs(1U);

        status = W5500_Socket0_GetStatus();

        if(status == W5500_Sn_SR_LISTEN)
        {
            return 1U;
        }
    }

    return 0U;
}

uint16_t W5500_Socket0_GetRxSize(void)
{
    uint8_t data[2];

    W5500_ReadRegisters(
        W5500_S0_RX_RSR,
        W5500_BSB_SOCKET0,
        data,
        2U
    );

    return (uint16_t)(
        ((uint16_t)data[0] << 8U) |
        data[1]
    );
}


uint16_t W5500_Socket0_Receive(
    uint8_t *buffer,
    uint16_t bufferSize
)
{
    uint8_t rxRdData[2];
    uint16_t rxSize;
    uint16_t rxReadPtr;
    uint16_t firstPart;
    uint16_t secondPart;

    if ((buffer == NULL) || (bufferSize == 0U))
    {
        return 0U;
    }

    /* Check how much data is available */
    rxSize = W5500_Socket0_GetRxSize();

    if (rxSize == 0U)
    {
        return 0U;
    }

    /* Never write beyond caller's buffer */
    if (rxSize > bufferSize)
    {
        rxSize = bufferSize;
    }

    /*
     * Read Socket 0 RX read pointer.
     */
    W5500_ReadRegisters(
        W5500_S0_RX_RD,
        W5500_BSB_SOCKET0,
        rxRdData,
        2U
    );

    rxReadPtr =
        (uint16_t)(((uint16_t)rxRdData[0] << 8U) |
                   rxRdData[1]);

    /*
     * Socket 0 RX memory = 2 KB.
     * Therefore the address wraps every 0x0800 bytes.
     */
    firstPart =
        (uint16_t)(0x0800U - (rxReadPtr & 0x07FFU));

    if (firstPart > rxSize)
    {
        firstPart = rxSize;
    }

    secondPart =
        (uint16_t)(rxSize - firstPart);

    /*
     * Read first contiguous section.
     */
    W5500_ReadRegisters(
        (uint16_t)(rxReadPtr & 0x07FFU),
        3U,
        buffer,
        firstPart
    );

    /*
     * If the pointer wrapped, read remaining bytes
     * from the beginning of the RX buffer.
     */
    if (secondPart > 0U)
    {
        W5500_ReadRegisters(
            0U,
            3U,
            &buffer[firstPart],
            secondPart
        );
    }

    /*
     * Advance Sn_RX_RD.
     */
    rxReadPtr =
        (uint16_t)(rxReadPtr + rxSize);

    rxRdData[0] =
        (uint8_t)(rxReadPtr >> 8U);

    rxRdData[1] =
        (uint8_t)(rxReadPtr & 0xFFU);

    W5500_WriteRegisters(
        W5500_S0_RX_RD,
        W5500_BSB_SOCKET0,
        rxRdData,
        2U
    );

    /*
     * Notify W5500 that the RX data has been consumed.
     */
    {
        uint8_t command = W5500_S0_RECV;

        W5500_WriteRegisters(
            W5500_S0_CR,
            W5500_BSB_SOCKET0,
            &command,
            1U
        );
    }

    return rxSize;
}

uint16_t W5500_Socket0_GetTxFreeSize(void)
{
    uint8_t data[2];

    W5500_ReadRegisters(
        W5500_S0_TX_FSR,
        W5500_BSB_SOCKET0,
        data,
        2U
    );

    return (uint16_t)(
        ((uint16_t)data[0] << 8U) |
        data[1]
    );
}

uint16_t W5500_Socket0_Send(
    const uint8_t *buffer,
    uint16_t length
)
{
    uint8_t txWrData[2];
    uint16_t txFreeSize;
    uint16_t txWritePtr;
    uint16_t firstPart;
    uint16_t secondPart;

    if ((buffer == NULL) || (length == 0U))
    {
        return 0U;
    }

    txFreeSize = W5500_Socket0_GetTxFreeSize();

    if (txFreeSize == 0U)
    {
        return 0U;
    }

    if (length > txFreeSize)
    {
        length = txFreeSize;
    }

    /* Read current TX write pointer */
    W5500_ReadRegisters(
        W5500_S0_TX_WR,
        W5500_BSB_SOCKET0,
        txWrData,
        2U
    );

    txWritePtr =
        (uint16_t)(((uint16_t)txWrData[0] << 8U) |
                   txWrData[1]);

    /*
     * Socket 0 TX buffer = 2 KB.
     */
    firstPart =
        (uint16_t)(0x0800U - (txWritePtr & 0x07FFU));

    if (firstPart > length)
    {
        firstPart = length;
    }

    secondPart =
        (uint16_t)(length - firstPart);

    /* First contiguous section */
    W5500_WriteRegisters(
        (uint16_t)(txWritePtr & 0x07FFU),
        2U,
        buffer,
        firstPart
    );

    /* Wrapped section */
    if (secondPart > 0U)
    {
        W5500_WriteRegisters(
            0U,
            2U,
            &buffer[firstPart],
            secondPart
        );
    }

    /* Advance TX write pointer */
    txWritePtr =
        (uint16_t)(txWritePtr + length);

    txWrData[0] =
        (uint8_t)(txWritePtr >> 8U);

    txWrData[1] =
        (uint8_t)(txWritePtr & 0xFFU);

    W5500_WriteRegisters(
        W5500_S0_TX_WR,
        W5500_BSB_SOCKET0,
        txWrData,
        2U
    );

    /* Tell W5500 to transmit */
    {
        uint8_t command = W5500_S0_SEND;

        W5500_WriteRegisters(
            W5500_S0_CR,
            W5500_BSB_SOCKET0,
            &command,
            1U
        );
    }

    return length;
}
