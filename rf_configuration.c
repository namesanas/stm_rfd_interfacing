/*
 * config.c
 *
 *  Created on: 12-Sept-2026
 *      Author: Identium
 */


#include <config.h>

/*
 * ============================================================
 * GLOBAL PERIPHERAL HANDLES
 * ============================================================
 */

USART_Handle_t usart3;
USART_Handle_t usart1;

Silion_Handle_t silion;
SILION_ReaderConfig_t readerConfig;
SPI_Handle_t w5500Spi;

/*
 * ============================================================
 * SILION ENABLE GPIO
 *
 * PB0 = active HIGH
 * ============================================================
 */

static void SILION_Enable_GPIO_Init(void)
{
    GPIO_Handle_t gpio;

    gpio.pGPIOx = GPIOB;

    gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_0;
    gpio.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
    gpio.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    gpio.GPIO_PinConfig.GPIO_PuPdControl = GPIO_NO_PUPD;
    gpio.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;

    GPIO_Init(&gpio);

    GPIO_WriteToOutputPin(
        GPIOB,
        GPIO_PIN_NO_0,
        GPIO_PIN_SET
    );
}


/*
 * ============================================================
 * USART3 GPIO
 *
 * PB10 -> USART3_TX -> SILION RX
 * PB11 -> USART3_RX <- SILION TX
 * ============================================================
 */

static void USART3_GPIO_Init(void)
{
    GPIO_Handle_t gpio;

    /*
     * PB10 -> USART3_TX
     */
    gpio.pGPIOx = GPIOB;

    gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_10;
    gpio.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    gpio.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    gpio.GPIO_PinConfig.GPIO_PuPdControl = GPIO_PIN_PU;
    gpio.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    gpio.GPIO_PinConfig.GPIO_PinAltFunMode = 7;

    GPIO_Init(&gpio);

    /*
     * PB11 -> USART3_RX
     */
    gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_11;
    gpio.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    gpio.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    gpio.GPIO_PinConfig.GPIO_PuPdControl = GPIO_PIN_PU;
    gpio.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    gpio.GPIO_PinConfig.GPIO_PinAltFunMode = 7;

    GPIO_Init(&gpio);
}


/*
 * ============================================================
 * USART1 GPIO
 *
 * PA9  -> USART1_TX -> ST-LINK VCP RX
 * PA10 -> USART1_RX <- ST-LINK VCP TX
 * ============================================================
 */

static void USART1_GPIO_Init(void)
{
    GPIO_Handle_t gpio;

    /*
     * PA9 -> USART1_TX
     */
    gpio.pGPIOx = GPIOA;

    gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_9;
    gpio.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    gpio.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    gpio.GPIO_PinConfig.GPIO_PuPdControl = GPIO_PIN_PU;
    gpio.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    gpio.GPIO_PinConfig.GPIO_PinAltFunMode = 7;

    GPIO_Init(&gpio);

    /*
     * PA10 -> USART1_RX
     */
    gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_10;
    gpio.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    gpio.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    gpio.GPIO_PinConfig.GPIO_PuPdControl = GPIO_PIN_PU;
    gpio.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    gpio.GPIO_PinConfig.GPIO_PinAltFunMode = 7;

    GPIO_Init(&gpio);
}


/*
 * ============================================================
 * USART3 INITIALIZATION
 * ============================================================
 */

static void USART3_Init(void)
{
    usart3.pUSARTx = USART3;

    usart3.USART_Config.USART_Mode =
        USART_MODE_TXRX;

    usart3.USART_Config.USART_Baud =
        USART_STD_BAUD_115200;

    usart3.USART_Config.USART_NoOfStopBits =
        USART_STOPBITS_1;

    usart3.USART_Config.USART_WordLength =
        USART_WORDLEN_8BITS;

    usart3.USART_Config.USART_ParityControl =
        USART_PARITY_DISABLE;

    usart3.USART_Config.USART_HWFlowControl =
        USART_HW_FLOW_CTRL_NONE;

    usart3.USART_Config.USART_OverSampling =
        USART_OVERSAMPLING_16;

    USART_Init(&usart3);
}


/*
 * ============================================================
 * USART1 INITIALIZATION
 * ============================================================
 */

static void USART1_Init(void)
{
    usart1.pUSARTx = USART1;

    usart1.USART_Config.USART_Mode =
        USART_MODE_TXRX;

    usart1.USART_Config.USART_Baud =
        USART_STD_BAUD_115200;

    usart1.USART_Config.USART_NoOfStopBits =
        USART_STOPBITS_1;

    usart1.USART_Config.USART_WordLength =
        USART_WORDLEN_8BITS;

    usart1.USART_Config.USART_ParityControl =
        USART_PARITY_DISABLE;

    usart1.USART_Config.USART_HWFlowControl =
        USART_HW_FLOW_CTRL_NONE;

    usart1.USART_Config.USART_OverSampling =
        USART_OVERSAMPLING_16;

    USART_Init(&usart1);
}


/*
 * ============================================================
 * READER CONFIGURATION
 * ============================================================
 */

static void SILION_ReaderConfig_Init(void)
{
    readerConfig.region =
        SILION_REGION_FULL_BAND;

    readerConfig.txAntenna =
        1U;

    readerConfig.rxAntenna =
        1U;

    readerConfig.readPower =
        3000U;

    readerConfig.writePower =
        3000U;

    readerConfig.tagProtocol =
        SILION_TAG_PROTOCOL_GEN2;

    readerConfig.session =
        SILION_SESSION_0;
}

/*
 * ============================================================
 * W5500 GPIO
 *
 * PB12 -> W5500 CS
 * PB13 -> SPI2_SCK
 * PB14 -> SPI2_MISO
 * PB15 -> SPI2_MOSI
 * PC0  -> W5500 RESET
 * ============================================================
 */

static void W5500_GPIO_Init(void)
{
    GPIO_Handle_t gpio;

    /*
     * ========================================================
     * Enable GPIO clocks
     * ========================================================
     */
    GPIOA_PCLK_EN();
    GPIOB_PCLK_EN();
    GPIOC_PCLK_EN();

    /*
     * ========================================================
     * W5500 CS -> PB12
     * ========================================================
     */
    gpio.pGPIOx = GPIOB;

    gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_12;
    gpio.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
    gpio.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    gpio.GPIO_PinConfig.GPIO_PuPdControl = GPIO_NO_PUPD;
    gpio.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;

    GPIO_Init(&gpio);

    /* CS inactive */
    GPIO_WriteToOutputPin(
        GPIOB,
        GPIO_PIN_NO_12,
        GPIO_PIN_SET
    );

    /*
     * ========================================================
     * SPI1 SCK -> PA5
     * ========================================================
     */
    gpio.pGPIOx = GPIOA;

    gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_5;
    gpio.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    gpio.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    gpio.GPIO_PinConfig.GPIO_PuPdControl = GPIO_NO_PUPD;
    gpio.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    gpio.GPIO_PinConfig.GPIO_PinAltFunMode = 5;

    GPIO_Init(&gpio);

    /*
     * ========================================================
     * SPI1 MISO -> PA6
     * ========================================================
     */
    gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
    gpio.GPIO_PinConfig.GPIO_PuPdControl = GPIO_PIN_PU;

    GPIO_Init(&gpio);

    /*
     * ========================================================
     * SPI1 MOSI -> PA7
     * ========================================================
     */
    gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
    gpio.GPIO_PinConfig.GPIO_PuPdControl = GPIO_NO_PUPD;

    GPIO_Init(&gpio);

    /*
     * ========================================================
     * W5500 RESET -> PC0
     * ========================================================
     */
    gpio.pGPIOx = GPIOC;

    gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_0;
    gpio.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
    gpio.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    gpio.GPIO_PinConfig.GPIO_PuPdControl = GPIO_NO_PUPD;
    gpio.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;

    GPIO_Init(&gpio);

    /* Keep W5500 out of reset */
    GPIO_WriteToOutputPin(
        GPIOC,
        GPIO_PIN_NO_0,
        GPIO_PIN_SET
    );
}

/*
 * ============================================================
 * W5500 SPI2 INITIALIZATION
 *
 * PB13 -> SPI2_SCK
 * PB14 -> SPI2_MISO
 * PB15 -> SPI2_MOSI
 *
 * SPI2:
 *   Master
 *   Full duplex
 *   8-bit
 *   CPOL = LOW
 *   CPHA = LOW
 *   Software NSS
 *   Conservative clock for initial bring-up
 * ============================================================
 */

static void W5500_SPI1_Init(void)
{
    w5500Spi.pSPIx = SPI1;

    w5500Spi.SPIConfig.SPI_DeviceMode =
        SPI_DEVICE_MODE_MASTER;

    w5500Spi.SPIConfig.SPI_BusConfig =
        SPI_BUS_CONFIG_FD;

    w5500Spi.SPIConfig.SPI_SclkSpeed =
        SPI_SCLK_SPEED_DIV8;

    w5500Spi.SPIConfig.SPI_DFF =
        SPI_DFF_8BITS;

    w5500Spi.SPIConfig.SPI_CPOL =
        SPI_CPOL_LOW;

    w5500Spi.SPIConfig.SPI_CPHA =
        SPI_CPHA_LOW;

    w5500Spi.SPIConfig.SPI_SSM =
        SPI_SSM_EN;

    SPI_Init(&w5500Spi);

    SPI_SSIConfig(
        SPI1,
        ENABLE
    );

    SPI_PeripheralControl(
        SPI1,
        ENABLE
    );
}

static void W5500_SPI1_GPIO_Init(void)
{
    GPIO_Handle_t gpio;

    gpio.pGPIOx = GPIOA;

    gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_5;
    gpio.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    gpio.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    gpio.GPIO_PinConfig.GPIO_PuPdControl = GPIO_NO_PUPD;
    gpio.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    gpio.GPIO_PinConfig.GPIO_PinAltFunMode = 5;

    GPIO_Init(&gpio);

    gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
    GPIO_Init(&gpio);

    gpio.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
    GPIO_Init(&gpio);
}



/*
 * ============================================================
 * RF CONFIGURATION INIT
 *
 * Hardware configuration only.
 * Timing/application sequencing remains in main.c for now.
 * ============================================================
 */

void RF_Configuration_Init(void)
{
    SILION_Enable_GPIO_Init();

    USART3_GPIO_Init();
    USART1_GPIO_Init();

    USART3_Init();
    USART1_Init();

    W5500_GPIO_Init();
    W5500_SPI1_Init();

    SILION_Init(
        &silion,
        &usart3
    );

    SILION_ReaderConfig_Init();

    /*
     * --------------------------------------------------------
     * USART INTERRUPTS
     * --------------------------------------------------------
     */

    USART_IRQPriorityConfig(
        IRQ_NO_USART3,
        5
    );

    USART_IRQInterruptConfig(
        IRQ_NO_USART3,
        ENABLE
    );

    USART_IRQPriorityConfig(
        IRQ_NO_USART1,
        5
    );

    USART_IRQInterruptConfig(
        IRQ_NO_USART1,
        ENABLE
    );

    /*
     * --------------------------------------------------------
     * CLEAR SILION UART STATE
     * --------------------------------------------------------
     */

    SILION_ClearRxQueue();
    SILION_ClearUartFlags();

    txComplete = 0U;
    rxComplete = 0U;

    /*
     * --------------------------------------------------------
     * START UART RX
     * --------------------------------------------------------
     */

    USART_ReceiveByteIT(&usart3);
    USART_ReceiveByteIT(&usart1);

}


