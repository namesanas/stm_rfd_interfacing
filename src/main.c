#include <stdint.h>
#include <stdio.h>

#include "stm32f429xx.h"
#include "stm32f429xx_driver_gpio.h"
#include "stm32f429xx_driver_uart.h"
#include "jrd100.h"

extern void initialise_monitor_handles(void);


/*
 * ============================================================
 * STM32F429ZI + JRD-100 REAL UART TEST
 *
 * USART3:
 *
 * PB10 / USART3_TX  -> JRD-100 RX
 * PB11 / USART3_RX  <- JRD-100 TX
 * PB0              -> JRD-100 ENABLE
 * GND               <-> GND
 *
 * PB0 is ACTIVE HIGH.
 *
 * ============================================================
 */


/*
 * ============================================================
 * GLOBAL HANDLES
 * ============================================================
 */

USART_Handle_t usart3;
JRD100_Handle_t jrd100;


/*
 * ============================================================
 * APPLICATION FLAGS
 * ============================================================
 */

volatile uint8_t txComplete = 0;
volatile uint8_t rxComplete = 0;
volatile uint32_t rxError = 0;


/*
 * ============================================================
 * RX SOFTWARE QUEUE
 *
 * The USART ISR only stores received bytes in this queue.
 * JRD100_ProcessByte() runs from main context so the ISR
 * stays as short as possible.
 * ============================================================
 */

#define RX_QUEUE_SIZE 256

volatile uint8_t rxQueue[RX_QUEUE_SIZE];

volatile uint16_t rxHead = 0;
volatile uint16_t rxTail = 0;
volatile uint16_t rxCount = 0;

volatile uint32_t rxOverflow = 0;


/*
 * ============================================================
 * JRD-100 ENABLE GPIO INITIALIZATION
 *
 * PB0 -> JRD-100 ENABLE
 *
 * Active HIGH:
 * GPIO HIGH = JRD-100 enabled
 * ============================================================
 */

static void JRD100_Enable_GPIO_Init(void)
{
    GPIO_Handle_t gpio;

    gpio.pGPIOx = GPIOB;

    gpio.GPIO_PinConfig.GPIO_PinNumber =
        GPIO_PIN_NO_0;

    gpio.GPIO_PinConfig.GPIO_PinMode =
        GPIO_MODE_OUT;

    gpio.GPIO_PinConfig.GPIO_PinSpeed =
        GPIO_SPEED_FAST;

    gpio.GPIO_PinConfig.GPIO_PuPdControl =
        GPIO_NO_PUPD;

    gpio.GPIO_PinConfig.GPIO_PinOPType =
        GPIO_OP_TYPE_PP;

    GPIO_Init(&gpio);

    /*
     * JRD-100 enable is active HIGH.
     */
    GPIO_WriteToOutputPin(
        GPIOB,
        GPIO_PIN_NO_0,
        GPIO_PIN_SET
    );
}


/*
 * ============================================================
 * USART3 GPIO INITIALIZATION
 *
 * PB10 -> USART3_TX -> JRD-100 RX
 * PB11 -> USART3_RX <- JRD-100 TX
 *
 * AF7
 * ============================================================
 */

static void USART3_GPIO_Init(void)
{
    GPIO_Handle_t gpio;


    /*
     * --------------------------------------------------------
     * PB10 -> USART3_TX
     * --------------------------------------------------------
     */

    gpio.pGPIOx = GPIOB;

    gpio.GPIO_PinConfig.GPIO_PinNumber =
        GPIO_PIN_NO_10;

    gpio.GPIO_PinConfig.GPIO_PinMode =
        GPIO_MODE_ALTFN;

    gpio.GPIO_PinConfig.GPIO_PinSpeed =
        GPIO_SPEED_FAST;

    gpio.GPIO_PinConfig.GPIO_PuPdControl =
        GPIO_PIN_PU;

    gpio.GPIO_PinConfig.GPIO_PinOPType =
        GPIO_OP_TYPE_PP;

    gpio.GPIO_PinConfig.GPIO_PinAltFunMode =
        7;

    GPIO_Init(&gpio);


    /*
     * --------------------------------------------------------
     * PB11 -> USART3_RX
     * --------------------------------------------------------
     */

    gpio.GPIO_PinConfig.GPIO_PinNumber =
        GPIO_PIN_NO_11;

    gpio.GPIO_PinConfig.GPIO_PinMode =
        GPIO_MODE_ALTFN;

    gpio.GPIO_PinConfig.GPIO_PinSpeed =
        GPIO_SPEED_FAST;

    gpio.GPIO_PinConfig.GPIO_PuPdControl =
        GPIO_PIN_PU;

    gpio.GPIO_PinConfig.GPIO_PinOPType =
        GPIO_OP_TYPE_PP;

    gpio.GPIO_PinConfig.GPIO_PinAltFunMode =
        7;

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


    USART_Init(&usart3);
}


/*
 * ============================================================
 * USART3 IRQ HANDLER
 * ============================================================
 */

void USART3_IRQHandler(void)
{
    USART_IRQHandling(&usart3);
}


/*
 * ============================================================
 * USART APPLICATION EVENT CALLBACK
 *
 * RX interrupt only puts the received byte into the software
 * queue. The JRD-100 parser is executed from main().
 *
 * IMPORTANT:
 * Do not put printf() here.
 * ============================================================
 */

void USART_ApplicationEventCallback(
        USART_Handle_t *pUSARTHandle,
        uint8_t AppEvent,
        uint8_t receivedByte)
{
    (void)pUSARTHandle;


    /*
     * --------------------------------------------------------
     * ONE BYTE RECEIVED
     * --------------------------------------------------------
     */

    if(AppEvent == USART_EVENT_RX_BYTE)
    {
        /*
         * Put byte into software RX queue.
         *
         * Keep this extremely short because this function
         * executes inside the USART interrupt.
         */
        if(rxCount < RX_QUEUE_SIZE)
        {
            rxQueue[rxHead] = receivedByte;

            rxHead++;

            if(rxHead >= RX_QUEUE_SIZE)
            {
                rxHead = 0;
            }

            rxCount++;
        }
        else
        {
            /*
             * Software queue is full.
             */
            rxOverflow = 1;
        }
    }


    /*
     * --------------------------------------------------------
     * TRANSMISSION COMPLETE
     * --------------------------------------------------------
     */

    else if(AppEvent == USART_EVENT_TX_CMPLT)
    {
        txComplete = 1;
    }


    /*
     * --------------------------------------------------------
     * FIXED-LENGTH RX COMPLETE
     * --------------------------------------------------------
     */

    else if(AppEvent == USART_EVENT_RX_CMPLT)
    {
        rxComplete = 1;
    }


    /*
     * --------------------------------------------------------
     * USART ERRORS
     * --------------------------------------------------------
     */

    else if(AppEvent == USART_EVENT_ORE ||
            AppEvent == USART_EVENT_FE  ||
            AppEvent == USART_EVENT_NE  ||
            AppEvent == USART_EVENT_PE)
    {
        rxError = 1;
    }
}


/*
 * ============================================================
 * PRINT JRD-100 FRAME
 * ============================================================
 */

static void PrintJRD100Frame(void)
{
    uint16_t i;

    printf("\r\n");
    printf("========================================\r\n");
    printf("JRD-100 FRAME RECEIVED\r\n");
    printf("========================================\r\n");

    printf("Length: %u\r\n", jrd100.rxIndex);

    printf("Frame:\r\n");

    for(i = 0; i < jrd100.rxIndex; i++)
    {
        printf("%02X ", jrd100.rxBuffer[i]);
    }

    printf("\r\n");
}


/*
 * ============================================================
 * MAIN
 * ============================================================
 */

int main(void)
{
    uint16_t i;
    uint8_t frameValid;


    /*
     * --------------------------------------------------------
     * DEBUG CONSOLE
     * --------------------------------------------------------
     */

    initialise_monitor_handles();

    printf("\r\n");
    printf("========================================\r\n");
    printf(" STM32F429ZI + JRD-100 TEST\r\n");
    printf("========================================\r\n");


    /*
     * --------------------------------------------------------
     * 1. Initialize JRD-100 ENABLE GPIO
     *
     * PB0 = HIGH -> JRD-100 ENABLED
     * --------------------------------------------------------
     */

    JRD100_Enable_GPIO_Init();


    /*
     * --------------------------------------------------------
     * 2. Initialize USART3 GPIO
     * --------------------------------------------------------
     */

    USART3_GPIO_Init();


    /*
     * --------------------------------------------------------
     * 3. Initialize USART3
     * --------------------------------------------------------
     */

    USART3_Init();


    /*
     * --------------------------------------------------------
     * 4. Initialize JRD-100 driver
     * --------------------------------------------------------
     */

    JRD100_Init(
        &jrd100,
        &usart3
    );


    /*
     * --------------------------------------------------------
     * 5. Configure USART3 IRQ priority
     * --------------------------------------------------------
     */

    USART_IRQPriorityConfig(
        IRQ_NO_USART3,
        5
    );


    /*
     * --------------------------------------------------------
     * 6. Enable USART3 interrupt
     * --------------------------------------------------------
     */

    USART_IRQInterruptConfig(
        IRQ_NO_USART3,
        ENABLE
    );


    /*
     * --------------------------------------------------------
     * 7. Initialize RX software queue
     * --------------------------------------------------------
     */

    for(i = 0; i < RX_QUEUE_SIZE; i++)
    {
        rxQueue[i] = 0;
    }

    rxHead = 0;
    rxTail = 0;
    rxCount = 0;
    rxOverflow = 0;


    /*
     * --------------------------------------------------------
     * 8. Clear application flags
     * --------------------------------------------------------
     */

    txComplete = 0;
    rxComplete = 0;
    rxError = 0;


    /*
     * --------------------------------------------------------
     * 9. START RX
     *
     * Enable byte-based RX interrupts.
     * --------------------------------------------------------
     */

    USART_ReceiveByteIT(&usart3);


    /*
     * --------------------------------------------------------
     * 10. SEND JRD-100 GET READER INFO COMMAND
     * --------------------------------------------------------
     */

    txComplete = 0;

    JRD100_GetReaderInfo(
        &jrd100,
        0x00
    );


    /*
     * --------------------------------------------------------
     * 11. Wait until transmission is completely finished.
     *
     * RX interrupts continue running while we wait.
     * --------------------------------------------------------
     */

    while(txComplete == 0)
    {
        /*
         * USART interrupts handle TX and RX.
         */
    }


    printf("\r\n");
    printf("JRD-100 command transmitted.\r\n");
    printf("Waiting for JRD-100 response...\r\n");


    /*
     * --------------------------------------------------------
     * 12. MAIN LOOP
     * --------------------------------------------------------
     */

    while(1)
    {
        /*
         * ----------------------------------------------------
         * Process queued bytes outside interrupt context.
         * ----------------------------------------------------
         */

        while(rxCount > 0)
        {
            uint8_t byte;

            byte = rxQueue[rxTail];

            rxTail++;

            if(rxTail >= RX_QUEUE_SIZE)
            {
                rxTail = 0;
            }

            rxCount--;

            JRD100_ProcessByte(
                &jrd100,
                byte
            );
        }


        /*
         * ----------------------------------------------------
         * RX queue overflow
         * ----------------------------------------------------
         */

        if(rxOverflow)
        {
            printf("\r\nRX queue overflow!\r\n");
            rxOverflow = 0;
        }


        /*
         * ----------------------------------------------------
         * USART error
         * ----------------------------------------------------
         */

        if(rxError)
        {
            printf("\r\nUSART RX ERROR!\r\n");
            rxError = 0;
        }


        /*
         * ----------------------------------------------------
         * JRD-100 frame ready
         * ----------------------------------------------------
         */

        if(JRD100_IsFrameReady(&jrd100))
        {
            printf("\r\n");
            printf("JRD-100 frameReady = 1\r\n");

            frameValid =
                JRD100_ValidateFrame(&jrd100);

            if(frameValid)
            {
                printf("Checksum: VALID\r\n");
            }
            else
            {
                printf("Checksum: INVALID\r\n");
            }

            PrintJRD100Frame();

            JRD100_ClearFrame(&jrd100);

            printf("Waiting for next frame...\r\n");
        }
    }
}

