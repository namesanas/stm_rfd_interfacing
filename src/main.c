
#include <stdint.h>
#include <stdio.h>

#include "stm32f429xx.h"
#include "stm32f429xx_driver_gpio.h"
#include "stm32f429xx_driver_uart.h"
#include "silion.h"

extern void initialise_monitor_handles(void);


/*
 * ============================================================
 * GLOBALS
 * ============================================================
 */

USART_Handle_t usart3;
Silion_Handle_t silion;


/*
 * ============================================================
 * FLAGS
 * ============================================================
 */

volatile uint8_t txComplete = 0;
volatile uint8_t rxByteReceived = 0;
volatile uint8_t rxError = 0;


/*
 * ============================================================
 * DEBUG RX BUFFER
 *
 * We don't use a queue here.
 *
 * Every received byte is immediately passed to the Silion
 * parser from the UART callback.
 * ============================================================
 */

#define SILION_DEBUG_RX_BUFFER_SIZE 256

volatile uint8_t debugRxBuffer[
    SILION_DEBUG_RX_BUFFER_SIZE
];

volatile uint16_t debugRxIndex = 0;


/*
 * ============================================================
 * USART3 GPIO
 * ============================================================
 *
 * PB10 -> USART3_TX -> SILION RX
 * PB11 <- USART3_RX <- SILION TX
 *
 * AF7
 *
 * ============================================================
 */

static void USART3_GPIO_Init(void)
{
    GPIO_Handle_t gpio;


    /*
     * PB10 TX
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
     * PB11 RX
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
    usart3.pUSARTx =
        USART3;


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
 * UART CALLBACK
 * ============================================================
 *
 * IMPORTANT:
 *
 * Every byte received by USART3 is immediately passed into
 * SILION_ProcessByte().
 *
 * We don't wait for a fixed RX length because Silion replies
 * are length-prefixed.
 *
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
     * RX BYTE
     * --------------------------------------------------------
     */

    if(AppEvent == USART_EVENT_RX_BYTE)
    {
        rxByteReceived = 1;


        /*
         * Save raw byte for debugging.
         */
        if(debugRxIndex <
           SILION_DEBUG_RX_BUFFER_SIZE)
        {
            debugRxBuffer[
                debugRxIndex++
            ] =
                receivedByte;
        }


        /*
         * Immediately feed byte to Silion parser.
         */
        SILION_ProcessByte(
            &silion,
            receivedByte
        );


        /*
         * IMPORTANT:
         *
         * ReceiveByteIT() uses continuous RX mode,
         * so DO NOT call ReceiveByteIT() again here.
         */
    }


    /*
     * --------------------------------------------------------
     * TX COMPLETE
     * --------------------------------------------------------
     */

    else if(AppEvent ==
            USART_EVENT_TX_CMPLT)
    {
        txComplete = 1;
    }


    /*
     * --------------------------------------------------------
     * USART ERROR
     * --------------------------------------------------------
     */

    else if(
        AppEvent == USART_EVENT_ORE ||
        AppEvent == USART_EVENT_FE  ||
        AppEvent == USART_EVENT_NE  ||
        AppEvent == USART_EVENT_PE
    )
    {
        rxError = 1;
    }
}


/*
 * ============================================================
 * PRINT RAW RX BYTES
 * ============================================================
 */

static void PrintRawRX(void)
{
    uint16_t i;


    printf("\r\n");
    printf("RAW RX (%u bytes):\r\n",
           debugRxIndex);


    for(
        i = 0;
        i < debugRxIndex;
        i++
    )
    {
        printf(
            "%02X ",
            debugRxBuffer[i]
        );


        if(
            ((i + 1U) % 16U) == 0U
        )
        {
            printf("\r\n");
        }
    }


    printf("\r\n");
}


/*
 * ============================================================
 * PRINT SILION RESPONSE
 * ============================================================
 */

static void PrintSilionResponse(void)
{
    uint16_t i;


    printf("\r\n");
    printf("========================================\r\n");
    printf(" SILION RESPONSE\r\n");
    printf("========================================\r\n");


    printf(
        "Length  : %u\r\n",
        silion.rxIndex
    );


    printf(
        "Command : 0x%02X\r\n",
        SILION_GetCommand(&silion)
    );


    printf(
        "Status  : 0x%04X\r\n",
        SILION_GetStatus(&silion)
    );


    printf(
        "Frame   : "
    );


    for(
        i = 0;
        i < silion.rxIndex;
        i++
    )
    {
        printf(
            "%02X ",
            silion.rxBuffer[i]
        );
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
    initialise_monitor_handles();


    printf("\r\n");
    printf("========================================\r\n");
    printf(" STM32F429ZI + SILION SIM3500\r\n");
    printf(" GET VERSION TEST\r\n");
    printf("========================================\r\n");


    /*
     * --------------------------------------------------------
     * 1. GPIO
     * --------------------------------------------------------
     */

    USART3_GPIO_Init();


    /*
     * --------------------------------------------------------
     * 2. USART3
     * --------------------------------------------------------
     */

    USART3_Init();


    /*
     * --------------------------------------------------------
     * 3. Initialise Silion parser
     * --------------------------------------------------------
     */

    SILION_Init(
        &silion,
        &usart3
    );


    /*
     * --------------------------------------------------------
     * 4. USART interrupt priority
     * --------------------------------------------------------
     */

    USART_IRQPriorityConfig(
        IRQ_NO_USART3,
        5
    );


    /*
     * --------------------------------------------------------
     * 5. Enable USART3 IRQ
     * --------------------------------------------------------
     */

    USART_IRQInterruptConfig(
        IRQ_NO_USART3,
        ENABLE
    );


    /*
     * --------------------------------------------------------
     * 6. Reset software state
     * --------------------------------------------------------
     */

    txComplete = 0;
    rxByteReceived = 0;
    rxError = 0;

    debugRxIndex = 0;


    /*
     * --------------------------------------------------------
     * 7. ENABLE RX BEFORE TX
     * --------------------------------------------------------
     *
     * This is important.
     *
     * The module can answer very quickly after receiving the
     * command, so RX must already be armed.
     *
     * --------------------------------------------------------
     */

    USART_ReceiveByteIT(
        &usart3
    );


    /*
     * --------------------------------------------------------
     * 8. SEND GET VERSION
     * --------------------------------------------------------
     *
     * PySilion protocol:
     *
     * FF 00 03 1D 0C
     *
     * 0x03 = Get Version
     *
     * --------------------------------------------------------
     */

    printf(
        "\r\nSending SILION GET VERSION...\r\n"
    );


    txComplete = 0;


    if(
        SILION_GetVersion(
            &silion
        ) == 0
    )
    {
        printf(
            "ERROR: failed to start TX\r\n"
        );


        while(1)
        {
        }
    }


    /*
     * --------------------------------------------------------
     * 9. WAIT FOR TX COMPLETE
     * --------------------------------------------------------
     *
     * Give ourselves a finite timeout instead of an
     * infinite loop.
     * --------------------------------------------------------
     */

    {
        volatile uint32_t timeout =
            50000000UL;


        while(
            txComplete == 0 &&
            timeout--
        )
        {
        }


        if(txComplete == 0)
        {
            printf(
                "ERROR: TX COMPLETE TIMEOUT\r\n"
            );


            while(1)
            {
            }
        }
    }


    printf(
        "TX complete.\r\n"
    );


    printf(
        "Waiting for SILION response...\r\n"
    );


    /*
     * --------------------------------------------------------
     * 10. WAIT FOR COMPLETE SILION FRAME
     * --------------------------------------------------------
     *
     * Silion parser sets frameReady after:
     *
     *   FF
     *   LEN
     *   CMD
     *   STATUS MSB
     *   STATUS LSB
     *   DATA
     *   CRC MSB
     *   CRC LSB
     *
     * --------------------------------------------------------
     */

    {
        volatile uint32_t timeout =
            100000000UL;


        while(
            !SILION_IsFrameReady(&silion) &&
            !silion.frameError &&
            !rxError &&
            timeout--
        )
        {
        }


        /*
         * No bytes at all.
         */
        if(
            rxByteReceived == 0
        )
        {
            printf(
                "\r\nERROR: NO RX BYTES RECEIVED\r\n"
            );


            printf(
                "The module did not produce a byte.\r\n"
            );


            while(1)
            {
            }
        }


        /*
         * UART error.
         */
        if(rxError)
        {
            printf(
                "\r\nERROR: UART RX ERROR\r\n"
            );


            PrintRawRX();


            while(1)
            {
            }
        }


        /*
         * Parser/CRC error.
         */
        if(silion.frameError)
        {
            printf(
                "\r\nERROR: SILION FRAME ERROR\r\n"
            );


            PrintRawRX();


            while(1)
            {
            }
        }


        /*
         * Timeout after receiving bytes.
         */
        if(
            !SILION_IsFrameReady(&silion)
        )
        {
            printf(
                "\r\nERROR: SILION RESPONSE TIMEOUT\r\n"
            );


            printf(
                "RX bytes received = %u\r\n",
                debugRxIndex
            );


            PrintRawRX();


            while(1)
            {
            }
        }
    }


    /*
     * --------------------------------------------------------
     * 11. PRINT RESPONSE
     * --------------------------------------------------------
     */

    PrintRawRX();


    PrintSilionResponse();


    /*
     * --------------------------------------------------------
     * 12. CHECK RESPONSE
     * --------------------------------------------------------
     */

    if(
        SILION_GetCommand(&silion)
        ==
        SILION_CMD_GET_VERSION
    )
    {
        printf(
            "GET VERSION COMMAND ECHO: OK\r\n"
        );
    }
    else
    {
        printf(
            "WARNING: unexpected command echo\r\n"
        );
    }


    if(
        SILION_GetStatus(&silion)
        ==
        SILION_STATUS_SUCCESS
    )
    {
        printf(
            "STATUS: SUCCESS\r\n"
        );
    }
    else
    {
        printf(
            "STATUS: 0x%04X\r\n",
            SILION_GetStatus(&silion)
        );
    }


    printf(
        "\r\nGET VERSION TEST COMPLETE\r\n"
    );


    /*
     * --------------------------------------------------------
     * STOP
     * --------------------------------------------------------
     */

    while(1)
    {
    }
}

