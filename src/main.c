
#include <config.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stm32f429xx.h"
#include "impinj.h"
#include "sillion_application.h"
#include "host_interface.h"
#include "w5500.h"


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

#define ETHERNET_RX_BUFFER_SIZE 256U

static uint8_t ethernetRxBuffer[ETHERNET_RX_BUFFER_SIZE];




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

#define HOST_TRANSPORT_USB       0U
#define HOST_TRANSPORT_ETHERNET  1U

volatile uint8_t hostRxSourceQueue[HOST_RX_QUEUE_SIZE];

/*
 * ============================================================
 * SILION ENABLE GPIO
 *
 * PB0 = active HIGH
 * ============================================================
 */



/*
 * ============================================================
 * USART3 GPIO
 *
 * PB10 -> USART3_TX -> SILION RX
 * PB11 -> USART3_RX <- SILION TX
 * ============================================================
 */


/*
 * ============================================================
 * USART3 INITIALIZATION
 * ============================================================
 */




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
                hostRxSourceQueue[hostRxHead] = HOST_TRANSPORT_USB;

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

 void SILION_ClearRxQueue(void)
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


void SILION_DelayMs(uint32_t delayMs)
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
    uint32_t startTime;

    startTime = g_msTick;

    while((g_msTick - startTime) < timeoutMs)
    {
        /*
         * Process any received bytes.
         */
        SILION_ProcessRxQueue();

        /*
         * UART errors.
         */
        if(rxORE || rxFE || rxNE || rxPE)
        {
            return -1;
        }

        /*
         * SILION parser rejected frame.

        if(silion.frameError)
        {
            return -2;
        }
	*/
        /*
         * Complete valid frame.
         */
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


    if(HOST_IsTagTransportEthernet())
    {
        W5500_Socket0_Send(
            (const uint8_t *)buffer,
            pos
        );
    }
    else
    {
        VCP_SendString(
            buffer
        );
    }
}

 static void Ethernet_PushToHostQueue(
     const uint8_t *data,
     uint16_t length
 )
 {
     uint16_t i;

     if(data == NULL)
     {
         return;
     }

     for(i = 0U; i < length; i++)
     {
         if(hostRxCount >= HOST_RX_QUEUE_SIZE)
         {
             hostRxOverflow = 1U;
             return;
         }

         hostRxQueue[hostRxHead] = data[i];
         hostRxSourceQueue[hostRxHead] = HOST_TRANSPORT_ETHERNET;

         hostRxHead++;

         if(hostRxHead >= HOST_RX_QUEUE_SIZE)
         {
             hostRxHead = 0U;
         }

         hostRxCount++;
     }
 }

 static void Ethernet_Task(void)
 {
     uint16_t received;

     received = W5500_Socket0_Receive(
         ethernetRxBuffer,
         sizeof(ethernetRxBuffer)
     );

     if(received > 0U)
     {
         Ethernet_PushToHostQueue(
             ethernetRxBuffer,
             received
         );
     }
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
     *keep in mind the problem is in both async and single poll so whatever change we did that causing them to only not send epc and read write commands arent working either
     * so both of application layers are working and
     * host late provides a concrete evidence of why was it not working before and why now working so
     * so the startup sequence concludes in pretty much all the same without any formalities
     * --------------------------------------------------------
     */
	initialise_monitor_handles();
    SILION_SysTick_Init();

    printf("\r\n");
    printf("========================================\r\n");
    printf(" STM32F429ZI + SILION / IMPINJ E310\r\n");
    printf(" STARTUP SEQUENCE TEST\r\n");
    printf("========================================\r\n");



    RF_Configuration_Init();


    SILION_DelayMs(100U);

    W5500_Init();
    W5500_SetNetworkConfig();


    uint8_t readIp[4];
    W5500_ReadRegisters(W5500_SIPR, 0U, readIp, 4U);
    uint8_t tcpServerStarted;

    tcpServerStarted = W5500_StartTCPServer(5000U);




/*
    while (1)
    {
        W5500_ReadRegisters(
            W5500_S0_SR,
            W5500_BSB_SOCKET0,
            &socketStatus,
            1U
        );

        if (socketStatus == 0x17U)
        {
            const uint8_t testMessage[] = "HELLO FROM STM32";
            uint16_t sentBytes;

            sentBytes = W5500_Socket0_Send(
                testMessage,
                sizeof(testMessage) - 1U
            );

            break;
        }
    }*/
    /*
     * --------------------------------------------------------
     * 4. SILION DRIVER can you find any bugs in this code the w5500version isnt updating to 0x04 for version
     * SILION_REGION_FULL_BAND
     * --------------------------------------------------------
     */

    SILION_Init(&silion, &usart3);

    readerConfig.region      = SILION_REGION_CHINA_1;

    readerConfig.txAntenna   = 1U;
    readerConfig.rxAntenna   = 1U;

    readerConfig.readPower   = 2500U;
    readerConfig.writePower  = 2500U;

    readerConfig.tagProtocol = SILION_TAG_PROTOCOL_GEN2;

    readerConfig.session     = SILION_SESSION_0;



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
    	Ethernet_Task();
        SILION_Application_Task();
        HOST_Interface_Task();
    }



}
