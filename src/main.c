#include <stdint.h>
#include <stdio.h>

#include "stm32f429xx.h"
#include "stm32f429xx_driver_gpio.h"
#include "stm32f429xx_driver_uart.h"
#include "jrd100.h"

extern void initialise_monitor_handles(void);

/*
 * ============================================================
 * JRD-100 connections
 *
 * PB0  -> JRD-100 EN
 *
 * PB10 -> USART3_TX -> JRD-100 RX
 * PB11 -> USART3_RX <- JRD-100 TX
 *
 * JRD-100 VCC -> external 5V
 * JRD-100 GND -> STM32 GND
 * ============================================================
 */


/*
 * Global handles
 */

USART_Handle_t usart3;

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

    gpio_en.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_0;
    gpio_en.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
    gpio_en.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    gpio_en.GPIO_PinConfig.GPIO_PuPdControl = GPIO_NO_PUPD;
    gpio_en.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;

    GPIO_Init(&gpio_en);
}


/*
 * Enable JRD-100
 *
 * EN is active HIGH.
 */

static void JRD100_Enable(void)
{
    GPIO_WriteToOutputPin(GPIOB,GPIO_PIN_NO_0,GPIO_PIN_SET);
}


/*
 * ============================================================
 * USART3 GPIO initialization
 *
 * PB10 = USART3_TX
 * PB11 = USART3_RX
 *
 * AF7
 * ============================================================
 */

static void USART3_GPIO_Init(void)
{
    GPIO_Handle_t gpio_usart;


    /*
     * PB10 - USART3 TX
     */

    gpio_usart.pGPIOx = GPIOB;

    gpio_usart.GPIO_PinConfig.GPIO_PinNumber =GPIO_PIN_NO_10;
    gpio_usart.GPIO_PinConfig.GPIO_PinMode =GPIO_MODE_ALTFN;
    gpio_usart.GPIO_PinConfig.GPIO_PinSpeed =GPIO_SPEED_FAST;
    gpio_usart.GPIO_PinConfig.GPIO_PuPdControl =GPIO_PIN_PU;
    gpio_usart.GPIO_PinConfig.GPIO_PinOPType =GPIO_OP_TYPE_PP;
    gpio_usart.GPIO_PinConfig.GPIO_PinAltFunMode =7;

    GPIO_Init(&gpio_usart);


    /*
     * PB11 - USART3 RX
     */

    gpio_usart.GPIO_PinConfig.GPIO_PinNumber =GPIO_PIN_NO_11;
    gpio_usart.GPIO_PinConfig.GPIO_PinMode =GPIO_MODE_ALTFN;
    gpio_usart.GPIO_PinConfig.GPIO_PinSpeed =GPIO_SPEED_FAST;
    gpio_usart.GPIO_PinConfig.GPIO_PuPdControl =GPIO_PIN_PU;
    gpio_usart.GPIO_PinConfig.GPIO_PinOPType =GPIO_OP_TYPE_PP;
    gpio_usart.GPIO_PinConfig.GPIO_PinAltFunMode =7;

    GPIO_Init(&gpio_usart);
}


/*
 * ============================================================
 * USART3 initialization
 * ============================================================
 */

static void USART3_Init(void)
{
    usart3.pUSARTx = USART3;


    /*
     * TX + RX
     */

    usart3.USART_Config.USART_Mode =
        USART_MODE_TXRX;


    /*
     * 115200 baud
     */

    usart3.USART_Config.USART_Baud =
        USART_STD_BAUD_115200;


    /*
     * 1 stop bit
     */

    usart3.USART_Config.USART_NoOfStopBits =
        USART_STOPBITS_1;


    /*
     * 8 data bits
     */

    usart3.USART_Config.USART_WordLength =
        USART_WORDLEN_8BITS;


    /*
     * No parity
     */

    usart3.USART_Config.USART_ParityControl =
        USART_PARITY_DISABLE;


    /*
     * No hardware flow control
     */

    usart3.USART_Config.USART_HWFlowControl =
        USART_HW_FLOW_CTRL_NONE;


    /*
     * Oversampling by 16
     */

    usart3.USART_Config.USART_OverSampling =
        USART_OVERSAMPLING_16;


    /*
     * Initialize USART3
     */

    USART_Init(&usart3);
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
	initialise_monitor_handles();

    uint8_t receivedByte;
    uint8_t frameValid;

    uint16_t i;


    printf("\r\n");
    printf("========================================\r\n");
    printf("STM32F429ZI + JRD-100 TEST\r\n");
    printf("========================================\r\n");


    /*
     * --------------------------------------------------------
     * 1. Configure JRD-100 EN
     * --------------------------------------------------------
     */

    printf("Initializing JRD-100 EN...\r\n");

    JRD100_EnablePin_Init();


    /*
     * --------------------------------------------------------
     * 2. Enable JRD-100
     * --------------------------------------------------------
     */

    printf("Enabling JRD-100...\r\n");

    JRD100_Enable();


    /*
     * Give reader time to start.
     */

    delay();


    /*
     * --------------------------------------------------------
     * 3. Configure USART3 GPIO
     * --------------------------------------------------------
     */

    printf("Initializing USART3 GPIO...\r\n");

    USART3_GPIO_Init();


    /*
     * --------------------------------------------------------
     * 4. Configure USART3
     * --------------------------------------------------------
     */

    printf("Initializing USART3...\r\n");

    USART3_Init();


    /*
     * --------------------------------------------------------
     * 5. Initialize JRD-100 driver
     * --------------------------------------------------------
     */

    printf("Initializing JRD-100 driver...\r\n");

    JRD100_Init(
        &jrd100,
        &usart3
    );


    printf("Initialization complete.\r\n");


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

    printf("\r\n");
    printf("Sending Get Reader Hardware Information...\r\n");

    if(JRD100_GetReaderInfo(
            &jrd100,
            0x00) == 0)
    {
       printf("ERROR: JRD-100 command transmission failed.\r\n");

        while(1)
        {
        }
    }


 //   printf("Command transmitted successfully.\r\n");
  //  printf("TX: BB 00 03 00 01 00 04 7E\r\n");


    /*
     * ========================================================
     * RECEIVE RESPONSE
     *
     * Blocking receive for now.
     *
     * USART_ReceiveData() waits for RXNE.
     *
     * Every received byte is passed to:
     *
     * JRD100_ProcessByte()
     * ========================================================
     */

//    printf("Waiting for JRD-100 response...\r\n");


    while(!JRD100_IsFrameReady(&jrd100))
    {
        USART_ReceiveData(
            &usart3,
            &receivedByte,
            1
        );

        JRD100_ProcessByte(
            &jrd100,
            receivedByte
        );
    }

    printf("\r\n");
    printf("Complete frame received!\r\n");


    /*
     * ========================================================
     * Validate received frame
     * ========================================================
     */

    frameValid = JRD100_ValidateFrame(
        &jrd100
    );


    if(frameValid)
    {
        printf("\r\n");
        printf("========================================\r\n");
        printf("JRD-100 RESPONSE: VALID\r\n");
        printf("========================================\r\n");


        printf("Frame length = %u\r\n",
               jrd100.rxIndex);


        printf("Raw frame: ");

        for(i = 0; i < jrd100.rxIndex; i++)
        {
            printf("%02X ",
                   jrd100.rxBuffer[i]);
        }

        printf("\r\n");


        printf("Checksum: VALID\r\n");

        printf("========================================\r\n");


        /*
         * TEST PASSED
         */

        while(1)
        {
        }
    }
    else
    {
        printf("\r\n");
        printf("========================================\r\n");
        printf("JRD-100 RESPONSE: CHECKSUM ERROR\r\n");
        printf("========================================\r\n");


        printf("Received frame: ");

        for(i = 0; i < jrd100.rxIndex; i++)
        {
            printf("%02X ",
                   jrd100.rxBuffer[i]);
        }

        printf("\r\n");


        while(1)
        {
        }
    }
}

