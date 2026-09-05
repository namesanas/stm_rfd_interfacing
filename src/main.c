
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stm32f429xx.h"
#include "stm32f429xx_driver_gpio.h"
#include "stm32f429xx_driver_uart.h"
#include "impinj.h"
#include "sillion_application.h"
#include "host_interface.h"

/*sinle polling commands
 * Cortex-M4 SysTick registers. The project uses a custom STM32
 * header without the CMSIS SysTick definitions, so access them
 * directly here.
 */
#define SYST_CSR   (*(volatile uint32_t *)0xE000E010UL)
#define SYST_RVR   (*(volatile uint32_t *)0xE000E014UL)
#define SYST_CVR   (*(volatile uint32_t *)0xE000E018UL)

#define SYST_CSR_ENABLE       (1UL << 0)
#define SYST_CSR_TICKINT      (1UL << 1)
#define SYST_CSR_CLKSOURCE    (1UL << 2)

#define SILION_ASYNC_MARKER_0  'M'
#define SILION_ASYNC_MARKER_1  'o'
#define SILION_ASYNC_MARKER_2  'd'
#define SILION_ASYNC_MARKER_3  'u'
#define SILION_ASYNC_MARKER_4  'l'
#define SILION_ASYNC_MARKER_5  'e'
#define SILION_ASYNC_MARKER_6  't'
#define SILION_ASYNC_MARKER_7  'e'
#define SILION_ASYNC_MARKER_8  'c'
#define SILION_ASYNC_MARKER_9  'h'

#define SILION_ASYNC_SUBCMD_START  0xAA48U

extern void initialise_monitor_handles(void);


/*
 * ============================================================
 * STM32F429ZI + SILION SIM3100/SIM3500 / IMPINJ E310
 *
 * USART3:
 *
 * PB10 / USART3_TX  -> SILION RX
 * PB11 / USART3_RX  <- SILION TX
 * PB0              -> SILION ENABLE
 * GND              <-> GND
 *
 * PB0 is ACTIVE HIGH.
 *
 * STARTUP SEQUENCE:
 *
 *     0x03  Get Version
 *     0x04  Boot Firmware
 *     0x0C  Get Run Phase
 *
 * Then stop.
 *
 * IMPORTANT:
 *
 * There are NO printf() calls between:
 *
 *     command TX
 *           and
 *     command response
 *
 * This avoids disturbing the UART transaction.
 *
 * ============================================================
 */
uint32_t testFrequencies[3] =
{
    900000U,
    910000U,
    920000U
};

/*
 * ============================================================
 * GLOBAL HANDLES
 * ============================================================
 */

USART_Handle_t usart3;
Silion_Handle_t silion;
USART_Handle_t usart1;
SILION_ReaderConfig_t readerConfig;

#define VCP_TX_BUFFER_SIZE 2048U

static uint8_t vcpTxBuffer[VCP_TX_BUFFER_SIZE];
static volatile uint16_t vcpTxHead = 0U;
static volatile uint16_t vcpTxTail = 0U;

static volatile uint8_t vcpTxBusy = 0U;
static volatile uint16_t vcpTxActiveLen = 0U;

SILION_Tag_t lastAsyncTag;
volatile uint8_t newAsyncTagAvailable = 0U;
/*
 * ============================================================
 * APPLICATION FLAGS
 * ============================================================
 */

volatile uint8_t txComplete = 0;
volatile uint8_t rxComplete = 0;


/*
 * UART error flags
 */

volatile uint8_t rxORE = 0;
volatile uint8_t rxFE  = 0;
volatile uint8_t rxNE  = 0;
volatile uint8_t rxPE  = 0;

volatile uint32_t silionAsyncPacketCount = 0;
volatile uint32_t silionAsyncBadFrameCount = 0;

/* 1 ms software time base */
volatile uint32_t g_msTick = 0;


/*
 * ============================================================
 * RX SOFTWARE QUEUE
 * ============================================================
 */

#define RX_QUEUE_SIZE 256
volatile uint8_t rxQueue[RX_QUEUE_SIZE];
volatile uint16_t rxHead = 0;
volatile uint16_t rxTail = 0;
volatile uint16_t rxCount = 0;
volatile uint32_t rxOverflow = 0;

#define HOST_RX_QUEUE_SIZE 256U
volatile uint8_t hostRxQueue[HOST_RX_QUEUE_SIZE];
volatile uint16_t hostRxHead = 0U;
volatile uint16_t hostRxTail = 0U;
volatile uint16_t hostRxCount = 0U;
volatile uint32_t hostRxOverflow = 0U;

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


    gpio.GPIO_PinConfig.GPIO_PinNumber =GPIO_PIN_NO_0;

    gpio.GPIO_PinConfig.GPIO_PinMode =GPIO_MODE_OUT;

    gpio.GPIO_PinConfig.GPIO_PinSpeed =GPIO_SPEED_FAST;

    gpio.GPIO_PinConfig.GPIO_PuPdControl =GPIO_NO_PUPD;

    gpio.GPIO_PinConfig.GPIO_PinOPType =GPIO_OP_TYPE_PP;


    GPIO_Init(&gpio);


    GPIO_WriteToOutputPin(GPIOB,GPIO_PIN_NO_0,GPIO_PIN_SET);
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

    gpio.GPIO_PinConfig.GPIO_PinNumber =GPIO_PIN_NO_10;

    gpio.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;

    gpio.GPIO_PinConfig.GPIO_PinSpeed =GPIO_SPEED_FAST;

    gpio.GPIO_PinConfig.GPIO_PuPdControl =GPIO_PIN_PU;

    gpio.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;

    gpio.GPIO_PinConfig.GPIO_PinAltFunMode =7;

    GPIO_Init(&gpio);


    /*
     * PB11 -> USART3_RX
     */
    gpio.GPIO_PinConfig.GPIO_PinNumber =GPIO_PIN_NO_11;

    gpio.GPIO_PinConfig.GPIO_PinMode =GPIO_MODE_ALTFN;

    gpio.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

    gpio.GPIO_PinConfig.GPIO_PuPdControl =GPIO_PIN_PU;

    gpio.GPIO_PinConfig.GPIO_PinOPType =GPIO_OP_TYPE_PP;

    gpio.GPIO_PinConfig.GPIO_PinAltFunMode =7;

    GPIO_Init(&gpio);
}

static void USART1_GPIO_Init(void)
{
    GPIO_Handle_t gpio;


    /*
     * PA9 -> USART1_TX -> ST-LINK VCP RX
     */
    gpio.pGPIOx = GPIOA;

    gpio.GPIO_PinConfig.GPIO_PinNumber =
        GPIO_PIN_NO_9;

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
     * PA10 -> USART1_RX
     *
     * We don't need RX yet, but initialize it so the
     * USART is configured as a normal TX/RX peripheral.
     */
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
}


/*
 * ============================================================
 * USART3 INITIALIZATION
 * ============================================================
 */

static void USART3_Init(void)
{
    usart3.pUSARTx =USART3;


    usart3.USART_Config.USART_Mode =USART_MODE_TXRX;

    usart3.USART_Config.USART_Baud = USART_STD_BAUD_115200;

    usart3.USART_Config.USART_NoOfStopBits =USART_STOPBITS_1;

    usart3.USART_Config.USART_WordLength =USART_WORDLEN_8BITS;

    usart3.USART_Config.USART_ParityControl =USART_PARITY_DISABLE;

    usart3.USART_Config.USART_HWFlowControl =USART_HW_FLOW_CTRL_NONE;

    usart3.USART_Config.USART_OverSampling =USART_OVERSAMPLING_16;

    USART_Init(&usart3);
}

static void USART1_Init(void)
{
    usart1.pUSARTx =
        USART1;


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
 * USART3 IRQ HANDLER
 * ============================================================
 */

void USART3_IRQHandler(void)
{
    USART_IRQHandling(&usart3);
}

void USART1_IRQHandler(void)
{
    USART_IRQHandling(&usart1);
}

/*
 * ============================================================
 * USART APPLICATION CALLBACK
 *
 * Keep ISR short.
 *
 * RX byte -> queue
 * TX complete -> flag
 * UART errors -> flags
 *
 * NO printf() HERE.
 * ============================================================
 */

void USART_ApplicationEventCallback(
        USART_Handle_t *pUSARTHandle,
        uint8_t AppEvent,
        uint8_t receivedByte)
{
    /*
     * ========================================================
     * USART1 = PC / ST-LINK VCP
     * ========================================================
     */
    if(pUSARTHandle == &usart1)
    {
        /*
         * ----------------------------------------------------
         * USART1 RX BYTE
         * ----------------------------------------------------
         */
        if(AppEvent == USART_EVENT_RX_BYTE)
        {
            if(hostRxCount < HOST_RX_QUEUE_SIZE)
            {
                hostRxQueue[hostRxHead] = receivedByte;

                hostRxHead++;

                if(hostRxHead >= HOST_RX_QUEUE_SIZE)
                {
                    hostRxHead = 0U;
                }

                hostRxCount++;
            }
            else
            {
                hostRxOverflow = 1U;
            }
        }

        /*
         * ----------------------------------------------------
         * USART1 TX COMPLETE
         *
         * Continue VCP TX ring.
         * ----------------------------------------------------
         */
        else if(AppEvent == USART_EVENT_TX_CMPLT)
        {
            vcpTxTail =
                (uint16_t)(
                    (vcpTxTail + vcpTxActiveLen)
                    % VCP_TX_BUFFER_SIZE
                );

            vcpTxActiveLen = 0U;

            if(vcpTxTail != vcpTxHead)
            {
                uint16_t len;

                if(vcpTxHead > vcpTxTail)
                {
                    len =
                        (uint16_t)(
                            vcpTxHead - vcpTxTail
                        );
                }
                else
                {
                    len =
                        (uint16_t)(
                            VCP_TX_BUFFER_SIZE - vcpTxTail
                        );
                }

                vcpTxActiveLen = len;

                USART_SendDataIT(
                    &usart1,
                    &vcpTxBuffer[vcpTxTail],
                    len
                );
            }
            else
            {
                vcpTxBusy = 0U;
            }
        }

        /*
         * USART1 events are completely handled above.
         */
        return;
    }


    /*
     * ========================================================
     * USART3 = SILION / E310
     * ========================================================
     */

    if(pUSARTHandle == &usart3)
    {
        /*
         * ----------------------------------------------------
         * USART3 RX BYTE
         * ----------------------------------------------------
         */
        if(AppEvent == USART_EVENT_RX_BYTE)
        {
            if(rxCount < RX_QUEUE_SIZE)
            {
                rxQueue[rxHead] = receivedByte;

                rxHead++;

                if(rxHead >= RX_QUEUE_SIZE)
                {
                    rxHead = 0U;
                }

                rxCount++;
            }
            else
            {
                rxOverflow = 1U;
            }
        }

        /*
         * ----------------------------------------------------
         * USART3 TX COMPLETE
         * ----------------------------------------------------
         */
        else if(AppEvent == USART_EVENT_TX_CMPLT)
        {
            txComplete = 1U;
        }

        /*
         * ----------------------------------------------------
         * USART3 RX COMPLETE
         * ----------------------------------------------------
         */
        else if(AppEvent == USART_EVENT_RX_CMPLT)
        {
            rxComplete = 1U;
        }

        /*
         * ----------------------------------------------------
         * USART3 UART ERRORS
         * ----------------------------------------------------
         */
        else if(AppEvent == USART_EVENT_ORE)
        {
            rxORE = 1U;
        }

        else if(AppEvent == USART_EVENT_FE)
        {
            rxFE = 1U;
        }

        else if(AppEvent == USART_EVENT_NE)
        {
            rxNE = 1U;
        }

        else if(AppEvent == USART_EVENT_PE)
        {
            rxPE = 1U;
        }
    }
}




/*
 * ============================================================
 * PROCESS RX QUEUE
 *
 * Every byte in the queue is passed to the SILION state
 * machine.
 * ============================================================
 */

void SILION_ProcessRxQueue(void)
{
    while(rxCount > 0)
    {
        uint8_t byte;


        byte =rxQueue[rxTail];
        rxTail++;


        if(rxTail >= RX_QUEUE_SIZE)
        {
            rxTail = 0;
        }


        rxCount--;


        SILION_ProcessByte(&silion,byte);
    }
}


/*
 * ============================================================
 * CLEAR SOFTWARE UART FLAGS
 * ============================================================
 */

void SILION_ClearUartFlags(void)
{
    rxORE = 0;
    rxFE  = 0;
    rxNE  = 0;
    rxPE  = 0;
}


/*
 * ============================================================
 * CLEAR RX QUEUE
 * ============================================================
 */

static void SILION_ClearRxQueue(void)
{
    uint16_t i;


    for(i = 0;i < RX_QUEUE_SIZE;i++)
    {
        rxQueue[i] = 0;
    }


    rxHead = 0;
    rxTail = 0;
    rxCount = 0;
    rxOverflow = 0;
}


/*
 * ============================================================
 * TIME BASE
 * ============================================================
 */

static uint32_t SILION_GetHCLKHz(void)
{
    uint32_t pclk1;
    uint32_t ppre1;
    uint32_t apb1Prescaler;

    pclk1 = RCC_GetPCLK1Value();
    ppre1 = (RCC->CFGR >> 10U) & 0x07U;

    switch(ppre1)
    {
        case 0U:
        case 1U:
        case 2U:
        case 3U:
            apb1Prescaler = 1U;
            break;

        case 4U:
            apb1Prescaler = 2U;
            break;

        case 5U:
            apb1Prescaler = 4U;
            break;

        case 6U:
            apb1Prescaler = 8U;
            break;

        default:
            apb1Prescaler = 16U;
            break;
    }

    return pclk1 * apb1Prescaler;
}


static void SILION_SysTick_Init(void)
{
    uint32_t hclkHz;

    hclkHz = SILION_GetHCLKHz();

    if(hclkHz < 1000U)
    {
        hclkHz = 16000000UL;
    }

    g_msTick = 0U;

    SYST_RVR = (hclkHz / 1000U) - 1U;
    SYST_CVR = 0U;
    SYST_CSR = SYST_CSR_CLKSOURCE |
               SYST_CSR_TICKINT   |
               SYST_CSR_ENABLE;
}


void SysTick_Handler(void)
{
    g_msTick++;
}


static void SILION_DelayMs(uint32_t delayMs)
{
    uint32_t start;

    start = g_msTick;

    while((g_msTick - start) < delayMs)
    {
        SILION_ProcessRxQueue();
    }
}


/*
 * ============================================================
 * WAIT FOR SILION RESPONSE
 *
 * timeout is now in REAL MILLISECONDS.
 * ============================================================
 */

int SILION_WaitForResponse(uint32_t timeoutMs)
{
    uint32_t start;

    start = g_msTick;

    while((g_msTick - start) < timeoutMs)
    {
        SILION_ProcessRxQueue();

        if(rxORE || rxFE || rxNE || rxPE)
        {
            return -1;
        }

        if(silion.frameError)
        {
            return -2;
        }

        if(SILION_IsFrameReady(&silion))
        {
            return 1;
        }
    }

    return 0;
}

/*
 * ============================================================
 * WAIT FOR TX COMPLETE
 *
 * timeout is now in REAL MILLISECONDS.
 * ============================================================
 */

 int SILION_WaitForTxComplete(uint32_t timeoutMs)
{
    uint32_t start;

    start = g_msTick;

    while((g_msTick - start) < timeoutMs)
    {
        SILION_ProcessRxQueue();

        if(rxORE || rxFE || rxNE || rxPE)
        {
            return -1;
        }

        if(txComplete)
        {
            return 1;
        }
    }

    return 0;
}


/*
 * ============================================================
 * PRINT FRAME
 * ============================================================
 */

void VCP_SendString(const char *text)
{
    uint16_t i = 0U;

    if (text == NULL)
    {
        return;
    }

    while (text[i] != '\0')
    {
        uint16_t nextHead =
            (uint16_t)((vcpTxHead + 1U) % VCP_TX_BUFFER_SIZE);

        /* Buffer full */
        if (nextHead == vcpTxTail)
        {
            return;
        }

        vcpTxBuffer[vcpTxHead] = (uint8_t)text[i];
        vcpTxHead = nextHead;
        i++;
    }

    /*
     * Start transmission if USART1 is currently idle.
     */
    if (!vcpTxBusy && (vcpTxTail != vcpTxHead))
    {
        uint16_t len;

        /*
         * Calculate contiguous bytes available from tail.
         */
        if (vcpTxHead > vcpTxTail)
        {
            len = (uint16_t)(vcpTxHead - vcpTxTail);
        }
        else
        {
            len = (uint16_t)(VCP_TX_BUFFER_SIZE - vcpTxTail);
        }

        vcpTxActiveLen = len;
        vcpTxBusy = 1U;

        USART_SendDataIT(
            &usart1,
            &vcpTxBuffer[vcpTxTail],
            len
        );
    }
}

 void VCP_SendTag(const SILION_Tag_t *tag)
{
    char buffer[160];

    uint16_t pos = 0U;


    pos +=
        sprintf(
            &buffer[pos],
            "TAG,EPC="
        );


    for(
        uint16_t i = 0U;
        i < tag->epcLengthBytes;
        i++
    )
    {
        pos +=
            sprintf(
                &buffer[pos],
                "%02X",
                tag->epc[i]
            );
    }


    pos +=
        sprintf(
            &buffer[pos],
            ",RSSI=%d,ANT=%u,FREQ=%lu,TIME=%lu\r\n",
            tag->rssi,
            tag->antenna,
            (unsigned long)tag->frequencyKHz,
            (unsigned long)tag->timestampMs
        );


    VCP_SendString(
        buffer
    );
}

/*
 * ============================================================
 * MAIN
 * ============================================================
 */

int main(void)
{
    /*
     * --------------------------------------------------------
     * DEBUG CONSOLE succesful now move ahead
     * okay both of them are working now lets go ahead
     * --------------------------------------------------------
     */
    initialise_monitor_handles();
    SILION_SysTick_Init();

    printf("\r\n");
    printf("========================================\r\n");
    printf(" STM32F429ZI + SILION / IMPINJ E310\r\n");
    printf(" STARTUP SEQUENCE TEST\r\n");
    printf("========================================\r\n");

 /*
     *
     * --------------------------------------------------------
     * 1. ENABLE SILION change the layout
     * --------------------------------------------------------
     */

    SILION_Enable_GPIO_Init();

    /*
     * Give the module time to initialize.
     */
    SILION_DelayMs(100U);


    /*
     * --------------------------------------------------------
     * 2. USART3 GPIO
     * --------------------------------------------------------
     */

    USART3_GPIO_Init();
    USART1_GPIO_Init();

    /*
     * --------------------------------------------------------
     * 3. USART3
     * --------------------------------------------------------
     */

    USART3_Init();
    USART1_Init();


    /*
     * --------------------------------------------------------
     * 4. SILION DRIVER
     * --------------------------------------------------------
     */

    SILION_Init(&silion, &usart3);

    readerConfig.region      = SILION_REGION_FULL_BAND;

    readerConfig.txAntenna   = 1U;
    readerConfig.rxAntenna   = 1U;

    readerConfig.readPower   = 3000U;
    readerConfig.writePower  = 3000U;

    readerConfig.tagProtocol = SILION_TAG_PROTOCOL_GEN2;

    readerConfig.session     = SILION_SESSION_0;
    /*
     * --------------------------------------------------------
     * 5. USART IRQ
     * --------------------------------------------------------
     */

    USART_IRQPriorityConfig(IRQ_NO_USART3,5);


    USART_IRQInterruptConfig(IRQ_NO_USART3,ENABLE );

    USART_IRQPriorityConfig(IRQ_NO_USART1,5);
    USART_IRQInterruptConfig(IRQ_NO_USART1,ENABLE);
    /*
     * --------------------------------------------------------
     * 6. CLEAR QUEUE / FLAGS
     * --------------------------------------------------------
     */

    SILION_ClearRxQueue();
    SILION_ClearUartFlags();


    txComplete = 0;
    rxComplete = 0;


    /*
     * --------------------------------------------------------
     * 7. START RX
     * --------------------------------------------------------
     */

    USART_ReceiveByteIT(&usart3);
    USART_ReceiveByteIT(&usart1);


    SILION_Application_Init(&silion);

    if(SILION_Application_ConfigureReader() == 0U)
    {
        VCP_SendString("ERROR,READER_CONFIG\r\n");

        while(1)
        {
        }
    }

    /*
     * --------------------------------------------------------
     * HOST INTERFACE
     * --------------------------------------------------------
     */

    HOST_Interface_Init();

    /*
     * --------------------------------------------------------
     * MAIN APPLICATION LOOP
     * --------------------------------------------------------
     */

    while(1)
    {
        SILION_Application_Task();
        HOST_Interface_Task();
    }



}
