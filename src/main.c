#include <stdint.h>

#include "stm32f429xx.h"
#include "stm32f429xx_driver_gpio.h"
#include "stm32f429xx_driver_uart.h"
#include "jrd100.h"


/*
 * ============================================================
 * JRD-100 connections
 *
 * PB0 -> JRD-100 EN
 *
 * PA2 -> USART2_TX -> JRD-100 RX
 * PA3 -> USART2_RX <- JRD-100 TX
 *
 * JRD-100 VCC -> external 5V
 * JRD-100 GND -> STM32 GND
 * ============================================================
 */


/*
 * Global handles
 */

USART_Handle_t usart2;

JRD100_Handle_t jrd100;


/*
 * ============================================================
 * JRD-100 Enable GPIO
 * ============================================================
 */

static void JRD100_EnablePin_Init(void)
{
    GPIO_Handle_t gpio_en;

    gpio_en.pGPIOx = GPIOB;

    gpio_en.GPIO_PinConfig.GPIO_PinNumber =
        GPIO_PIN_NO_0;

    gpio_en.GPIO_PinConfig.GPIO_PinMode =
        GPIO_MODE_OUT;

    gpio_en.GPIO_PinConfig.GPIO_PinSpeed =
        GPIO_SPEED_FAST;

    gpio_en.GPIO_PinConfig.GPIO_PuPdControl =
        GPIO_NO_PUPD;

    gpio_en.GPIO_PinConfig.GPIO_PinOPType =
        GPIO_OP_TYPE_PP;

    GPIO_Init(&gpio_en);
}


/*
 * Enable JRD-100
 *
 * EN is active HIGH.
 */

static void JRD100_Enable(void)
{
    GPIO_WriteToOutputPin(
        GPIOB,
        GPIO_PIN_NO_0,
        GPIO_PIN_SET
    );
}


/*
 * ============================================================
 * USART2 GPIO initialization
 *
 * PA2 = USART2_TX
 * PA3 = USART2_RX
 *
 * AF7
 * ============================================================
 */

static void USART2_GPIO_Init(void)
{
    GPIO_Handle_t gpio_usart;


    /*
     * PA2 - USART2 TX
     */

    gpio_usart.pGPIOx = GPIOA;

    gpio_usart.GPIO_PinConfig.GPIO_PinNumber =
        GPIO_PIN_NO_2;

    gpio_usart.GPIO_PinConfig.GPIO_PinMode =
        GPIO_MODE_ALTFN;

    gpio_usart.GPIO_PinConfig.GPIO_PinSpeed =
        GPIO_SPEED_FAST;

    gpio_usart.GPIO_PinConfig.GPIO_PuPdControl =
        GPIO_PIN_PU;

    gpio_usart.GPIO_PinConfig.GPIO_PinOPType =
        GPIO_OP_TYPE_PP;

    gpio_usart.GPIO_PinConfig.GPIO_PinAltFunMode =
        7;

    GPIO_Init(&gpio_usart);


    /*
     * PA3 - USART2 RX
     */

    gpio_usart.GPIO_PinConfig.GPIO_PinNumber =
        GPIO_PIN_NO_3;

    gpio_usart.GPIO_PinConfig.GPIO_PinMode =
        GPIO_MODE_ALTFN;

    gpio_usart.GPIO_PinConfig.GPIO_PinSpeed =
        GPIO_SPEED_FAST;

    gpio_usart.GPIO_PinConfig.GPIO_PuPdControl =
        GPIO_PIN_PU;

    gpio_usart.GPIO_PinConfig.GPIO_PinOPType =
        GPIO_OP_TYPE_PP;

    gpio_usart.GPIO_PinConfig.GPIO_PinAltFunMode =
        7;

    GPIO_Init(&gpio_usart);
}


/*
 * ============================================================
 * USART2 initialization
 * ============================================================
 */

static void USART2_Init(void)
{
    usart2.pUSARTx = USART2;


    /*
     * TX + RX
     */

    usart2.USART_Config.USART_Mode =
        USART_MODE_TXRX;


    /*
     * 115200 baud
     */

    usart2.USART_Config.USART_Baud =
        USART_STD_BAUD_115200;


    /*
     * 1 stop bit
     */

    usart2.USART_Config.USART_NoOfStopBits =
        USART_STOPBITS_1;


    /*
     * 8 data bits
     */

    usart2.USART_Config.USART_WordLength =
        USART_WORDLEN_8BITS;


    /*
     * No parity
     */

    usart2.USART_Config.USART_ParityControl =
        USART_PARITY_DISABLE;


    /*
     * No hardware flow control
     */

    usart2.USART_Config.USART_HWFlowControl =
        USART_HW_FLOW_CTRL_NONE;


    /*
     * Oversampling by 16
     *
     * IMPORTANT:
     * Don't leave this uninitialized.
     */

    usart2.USART_Config.USART_OverSampling =
        USART_OVERSAMPLING_16;


    /*
     * Initialize USART2
     */

    USART_Init(&usart2);


    /*
     * USART_Init() already enables the peripheral.
     */
}


/*
 * ============================================================
 * Small delay
 * ============================================================
 */

static void delay(void)
{
    volatile uint32_t i;

    for(i = 0; i < 1000000U; i++)
    {
        __asm volatile ("nop");
    }
}


/*
 * ============================================================
 * MAIN
 * ============================================================
 */

int main(void)
{
    uint8_t receivedByte;

    uint8_t frameValid;


    /*
     * --------------------------------------------------------
     * 1. Configure JRD-100 EN
     * --------------------------------------------------------
     */

    JRD100_EnablePin_Init();


    /*
     * --------------------------------------------------------
     * 2. Enable JRD-100
     * --------------------------------------------------------
     */

    JRD100_Enable();


    /*
     * Give reader time to start.
     */

    delay();


    /*
     * --------------------------------------------------------
     * 3. Configure USART2 GPIO
     * --------------------------------------------------------
     */

    USART2_GPIO_Init();


    /*
     * --------------------------------------------------------
     * 4. Configure USART2
     * --------------------------------------------------------
     */

    USART2_Init();


    /*
     * --------------------------------------------------------
     * 5. Initialize JRD-100 driver
     * --------------------------------------------------------
     */

    JRD100_Init(
        &jrd100,
        &usart2
    );


    /*
     * --------------------------------------------------------
     * 6. Give JRD-100 additional startup time
     * --------------------------------------------------------
     */

    delay();


    /*
     * ========================================================
     * TEST:
     *
     * Get Reader Hardware Information
     *
     * infoType = 0x00
     *
     * Expected TX:
     *
     * BB 00 03 00 01 00 04 7E
     * ========================================================
     */

    if(JRD100_GetReaderInfo(
            &jrd100,
            0x00) == 0)
    {
        /*
         * JRD100_SendFrame() failed.
         */

        while(1)
        {
        }
    }


    /*
     * ========================================================
     * RECEIVE RESPONSE
     *
     * IMPORTANT:
     *
     * We are NOT using interrupts yet.
     *
     * USART_ReceiveData() blocks until RXNE becomes set.
     *
     * Every received byte is passed to the JRD100 parser.
     * ========================================================
     */

    while(!JRD100_IsFrameReady(&jrd100))
    {
        /*
         * Receive exactly ONE byte.
         */

        USART_ReceiveData(
            &usart2,
            &receivedByte,
            1
        );


        /*
         * Give the received byte to
         * the JRD-100 state machine.
         */

        JRD100_ProcessByte(
            &jrd100,
            receivedByte
        );


        /*
         * If parser detected an error,
         * stop here for debugging.
         */

        if(jrd100.frameError)
        {
            while(1)
            {
            }
        }
    }


    /*
     * ========================================================
     * COMPLETE FRAME RECEIVED
     * ========================================================
     */

    frameValid = JRD100_ValidateFrame(
        &jrd100
    );


    if(frameValid)
    {
        /*
         * ====================================================
         * SUCCESS
         *
         * Put breakpoint here.
         *
         * Inspect:
         *
         * jrd100.rxBuffer[]
         * jrd100.rxIndex
         * jrd100.expectedLength
         * jrd100.frameReady
         * jrd100.frameError
         * ====================================================
         */

        while(1)
        {
        }
    }
    else
    {
        /*
         * ====================================================
         * CHECKSUM ERROR
         * ====================================================
         */

        while(1)
        {
        }
    }
}
