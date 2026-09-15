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
