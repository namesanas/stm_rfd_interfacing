#include "stm32f429xx.h"
#include "stm32f429xx_driver_uart.h"

void USART_PeriClockControl(USART_RegDef_t *pUSARTx,
                            uint8_t EnorDi)
{
    if(EnorDi == ENABLE)
    {
        if(pUSARTx == USART1)
            USART1_PCLK_EN();
        else if(pUSARTx == USART2)
            USART2_PCLK_EN();
        else if(pUSARTx == USART3)
            USART3_PCLK_EN();
        else if(pUSARTx == UART4)
            UART4_PCLK_EN();
        else if(pUSARTx == UART5)
            UART5_PCLK_EN();
        else if(pUSARTx == USART6)
            USART6_PCLK_EN();
        else if(pUSARTx == UART7)
            UART7_PCLK_EN();
        else if(pUSARTx == UART8)
            UART8_PCLK_EN();
    }
    else
    {
        if(pUSARTx == USART1)
            USART1_PCLK_DI();
        else if(pUSARTx == USART2)
            USART2_PCLK_DI();
        else if(pUSARTx == USART3)
            USART3_PCLK_DI();
        else if(pUSARTx == UART4)
            UART4_PCLK_DI();
        else if(pUSARTx == UART5)
            UART5_PCLK_DI();
        else if(pUSARTx == USART6)
            USART6_PCLK_DI();
        else if(pUSARTx == UART7)
            UART7_PCLK_DI();
        else if(pUSARTx == UART8)
            UART8_PCLK_DI();
    }
}

void USART_Init(USART_Handle_t *pUSARTHandle)
{
    uint32_t temp = 0;

    /*
     * Enable peripheral clock
     */
    USART_PeriClockControl(pUSARTHandle->pUSARTx, ENABLE);


    /*
     * Disable USART before configuration
     */
    pUSARTHandle->pUSARTx->CR1 &= ~(1 << USART_CR1_UE);


    /*
     * Configure word length
     */
    temp = pUSARTHandle->USART_Config.USART_WordLength;

    pUSARTHandle->pUSARTx->CR1 &= ~(1 << USART_CR1_M);

    if(temp == USART_WORDLEN_9BITS)
    {
        pUSARTHandle->pUSARTx->CR1 |=(1 << USART_CR1_M);
    }


    /*
     * Configure parity
     */
    pUSARTHandle->pUSARTx->CR1 &= ~((1 << USART_CR1_PCE) | (1 << USART_CR1_PS)
    );

    if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_EN_EVEN)
    {
        pUSARTHandle->pUSARTx->CR1 |= (1 << USART_CR1_PCE);
    }
    else if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_EN_ODD)
    {
        pUSARTHandle->pUSARTx->CR1 |= (1 << USART_CR1_PCE) | (1 << USART_CR1_PS);
    }


    /*
     * Configure number of stop bits
     */
    pUSARTHandle->pUSARTx->CR2 &= ~(0x3 << USART_CR2_STOP);

    pUSARTHandle->pUSARTx->CR2 |= (pUSARTHandle->USART_Config.USART_NoOfStopBits << USART_CR2_STOP);


    /*
     * Configure hardware flow control
     */
    pUSARTHandle->pUSARTx->CR3 &= ~((1 << USART_CR3_RTSE) | (1 << USART_CR3_CTSE)
    );

    if(pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_CTS)
    {
        pUSARTHandle->pUSARTx->CR3 |= (1 << USART_CR3_CTSE);
    }
    else if(pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_RTS)
    {
        pUSARTHandle->pUSARTx->CR3 |= (1 << USART_CR3_RTSE);
    }
    else if(pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_CTSRTS)
    {
        pUSARTHandle->pUSARTx->CR3 |= (1 << USART_CR3_RTSE) | (1 << USART_CR3_CTSE);
    }


    /*
     * Configure oversampling
     */
    pUSARTHandle->pUSARTx->CR1 &= ~(1 << USART_CR1_OVER8);

    if(pUSARTHandle->USART_Config.USART_OverSampling == USART_OVERSAMPLING_8)
    {
        pUSARTHandle->pUSARTx->CR1 |= (1 << USART_CR1_OVER8);
    }


    /*
     * Configure baud rate
     */
    USART_SetBaudRate(pUSARTHandle->pUSARTx,pUSARTHandle->USART_Config.USART_Baud);


    /*
     * Configure USART mode
     */
    pUSARTHandle->pUSARTx->CR1 &= ~((1 << USART_CR1_TE) | (1 << USART_CR1_RE)
    );

    if(pUSARTHandle->USART_Config.USART_Mode == USART_MODE_ONLY_TX)
    {
        pUSARTHandle->pUSARTx->CR1 |= (1 << USART_CR1_TE);
    }
    else if(pUSARTHandle->USART_Config.USART_Mode == USART_MODE_ONLY_RX)
    {
        pUSARTHandle->pUSARTx->CR1 |= (1 << USART_CR1_RE);
    }
    else if(pUSARTHandle->USART_Config.USART_Mode == USART_MODE_TXRX)
    {
        pUSARTHandle->pUSARTx->CR1 |= (1 << USART_CR1_TE) | (1 << USART_CR1_RE);
    }


    /*
     * Enable USART
     */
    pUSARTHandle->pUSARTx->CR1 |= (1 << USART_CR1_UE);
}

void USART_DeInit(USART_RegDef_t *pUSARTx)
{
    if(pUSARTx == USART1)
        USART1_REG_RESET();
    else if(pUSARTx == USART2)
        USART2_REG_RESET();
    else if(pUSARTx == USART3)
        USART3_REG_RESET();
    else if(pUSARTx == UART4)
        UART4_REG_RESET();
    else if(pUSARTx == UART5)
        UART5_REG_RESET();
    else if(pUSARTx == USART6)
        USART6_REG_RESET();
    else if(pUSARTx == UART7)
        UART7_REG_RESET();
    else if(pUSARTx == UART8)
        UART8_REG_RESET();
}

uint8_t USART_GetFlagStatus(USART_RegDef_t *pUSARTx,
                            uint8_t StatusFlagName)
{
    if(pUSARTx->SR & (1 << StatusFlagName))
    {
        return FLAG_SET;
    }

    return FLAG_RESET;
}

void USART_IRQInterruptConfig(uint8_t IRQNumber,uint8_t EnorDi)
{
   if(EnorDi == ENABLE)
	{
		if(IRQNumber <= 31)
		{
			*NVIC_ISER0 |= (1 << IRQNumber);
		}
		else if(IRQNumber > 31 && IRQNumber < 64)
		{
			*NVIC_ISER1 |= (1 << (IRQNumber % 32));
		}
		else if(IRQNumber >= 64 && IRQNumber < 96)
		{
			*NVIC_ISER2 |= (1 << (IRQNumber % 64));
		}
	}
	else
	{
		if(IRQNumber <= 31)
		{
			*NVIC_ICER0 |= (1 << IRQNumber);
		}
		else if(IRQNumber > 31 && IRQNumber < 64)
		{
			*NVIC_ICER1 |= (1 << (IRQNumber % 32));
		}
		else if(IRQNumber >= 64 && IRQNumber < 96)
		{
			*NVIC_ICER2 |= (1 << (IRQNumber % 64));
		}
	}
}


void USART_IRQPriorityConfig(uint8_t IRQNumber,uint32_t IRQPriority)
{
	 uint8_t iprx;
	    uint8_t section;
	    uint8_t shift_amount;

	    /*
	     * IRQ priority register index
	     */
	    iprx = IRQNumber / 4;

	    /*
	     * Which 8-bit priority field?
	     */
	    section = IRQNumber % 4;

	    /*
	     * STM32F4 implements 4 priority bits.
	     */
	    shift_amount =
	        (8 * section) + (8 - NO_PR_BITS_IMPLEMENTED);

	    NVIC_PR_BASE_ADDR[iprx] &=
	        ~(0xFF << shift_amount);

	    NVIC_PR_BASE_ADDR[iprx] |=
	        ((IRQPriority << shift_amount) & 0xFF);
}

void USART_SendData(USART_Handle_t *pUSARTHandle, uint8_t *pTxBuffer,uint32_t Len)
{
    uint16_t *pData;

    for(uint32_t i = 0; i < Len; i++)
    {
        /*
         * Wait until transmit data register is empty.
         */
        while(!USART_GetFlagStatus(pUSARTHandle->pUSARTx,USART_FLAG_TXE));

        /*
         * 9-bit word length
         */
        if(pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
        {
            pData = (uint16_t*)pTxBuffer;

            pUSARTHandle->pUSARTx->DR = (*pData & 0x01FF);

            /*
             * If parity is disabled, 9 data bits
             * are transmitted.
             *
             * Therefore advance by two bytes.
             */
            if(pUSARTHandle->USART_Config.USART_ParityControl== USART_PARITY_DISABLE)
            {
                pTxBuffer += 2;
            }
            else
            {
                /*
                 * With parity enabled only 8 data bits
                 * are available.
                 */
                pTxBuffer += 1;
            }
        }
        else
        {
            pUSARTHandle->pUSARTx->DR = (*pTxBuffer & 0xFF);

            pTxBuffer++;
        }
    }

    /*
     * Wait for transmission complete.
     */
    while(!USART_GetFlagStatus(pUSARTHandle->pUSARTx,USART_FLAG_TC));
}

void USART_ReceiveData(USART_Handle_t *pUSARTHandle,uint8_t *pRxBuffer,uint32_t Len)
{
    uint16_t *pData;

    for(uint32_t i = 0; i < Len; i++)
    {
        /*
         * Wait until receive data register is not empty.
         */
        while(!USART_GetFlagStatus(pUSARTHandle->pUSARTx,USART_FLAG_RXNE));


        /*
         * 9-bit word length
         */
        if(pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
        {
            if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
            {
                pData = (uint16_t*)pRxBuffer;

                *pData = (uint16_t)(pUSARTHandle->pUSARTx->DR & 0x01FF);

                pRxBuffer += 2;
            }
            else
            {
                *pRxBuffer = (uint8_t)(pUSARTHandle->pUSARTx->DR & 0xFF);

                pRxBuffer++;
            }
        }
        else
        {
            *pRxBuffer = (uint8_t)(pUSARTHandle->pUSARTx->DR & 0xFF);

            pRxBuffer++;
        }
    }
}

uint8_t USART_SendDataIT(USART_Handle_t *pUSARTHandle,uint8_t *pTxBuffer,uint32_t Len)

{
    if(pUSARTHandle->TxBusyState == USART_BUSY_IN_TX)
    {
        return USART_BUSY_IN_TX;
    }

    /*
     * Save TX buffer.
     */
    pUSARTHandle->pTxBuffer = pTxBuffer;

    /*
     * Save length.
     */
    pUSARTHandle->TxLen = Len;

    /*
     * Mark TX busy.
     */
    pUSARTHandle->TxBusyState = USART_BUSY_IN_TX;

    /*
     * Enable TXE interrupt.
     */
    pUSARTHandle->pUSARTx->CR1 |= (1 << USART_IRQ_TXE);

    return USART_BUSY_IN_TX;
}

uint8_t USART_ReceiveDataIT(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t Len)
{
    uint8_t state = pUSARTHandle->RxBusyState;

    if(state != USART_BUSY_IN_RX)
    {
        pUSARTHandle->pRxBuffer = pRxBuffer;
        pUSARTHandle->RxLen = Len;
        pUSARTHandle->RxBusyState = USART_BUSY_IN_RX;

        /* 1. Flush stale flags/bytes in SR and DR */
        (void)pUSARTHandle->pUSARTx->SR;
        (void)pUSARTHandle->pUSARTx->DR;

        /* 2. Enable Error Interrupt (EIE) in CR3 so ORE/FE forces an IRQ trigger */
        pUSARTHandle->pUSARTx->CR3 |= (1 << 0); // Bit 0 = EIE

        /* 3. Enable RXNE interrupt in CR1 */
        pUSARTHandle->pUSARTx->CR1 |= (1 << USART_IRQ_RXNE);
    }

    return state;
}

uint8_t USART_ReceiveByteIT(
    USART_Handle_t *pUSARTHandle)
{
    /*
     * Enable RXNE interrupt.
     */

    pUSARTHandle->pUSARTx->CR1 |=
        (1 << USART_IRQ_RXNE);


    /*
     * Enable USART error interrupt.
     *
     * This allows ORE/FE/NE to generate interrupts.
     */

    pUSARTHandle->pUSARTx->CR3 |=
        (1 << 0);


    /*
     * We are NOT using a fixed RX buffer here.
     */

    pUSARTHandle->pRxBuffer = 0;

    pUSARTHandle->RxLen = 0;

    pUSARTHandle->RxBusyState = USART_READY;


    return 1;
}

void USART_IRQHandling(USART_Handle_t *pUSARTHandle)
{
    uint8_t txe_status;
    uint8_t txeie_status;

    uint8_t rxne_status;
    uint8_t rxneie_status;

    /*
     * ========================================================
     * OVERRUN ERROR (ORE) HANDLING
     * ========================================================
     */

    uint8_t ore_status =
        USART_GetFlagStatus(
            pUSARTHandle->pUSARTx,
            USART_FLAG_ORE
        );

    uint8_t eie_status =
        (pUSARTHandle->pUSARTx->CR3 & (1 << 0));


    if(ore_status && eie_status)
    {
        /*
         * Clear ORE.
         */
        (void)pUSARTHandle->pUSARTx->SR;
        (void)pUSARTHandle->pUSARTx->DR;

        /*
         * Keep RX interrupt enabled.
         */
        pUSARTHandle->RxBusyState = USART_READY;

        USART_ApplicationEventCallback(
            pUSARTHandle,
            USART_EVENT_ORE,
            0
        );
    }


    /*
     * ========================================================
     * TXE INTERRUPT
     * ========================================================
     */

    txe_status =
        USART_GetFlagStatus(
            pUSARTHandle->pUSARTx,
            USART_FLAG_TXE
        );

    txeie_status =
        (pUSARTHandle->pUSARTx->CR1 &
        (1 << USART_IRQ_TXE))
        >> USART_IRQ_TXE;


    if(txe_status && txeie_status)
    {
        /*
         * Write next byte to DR.
         */

        pUSARTHandle->pUSARTx->DR =
            *(pUSARTHandle->pTxBuffer);


        pUSARTHandle->pTxBuffer++;

        pUSARTHandle->TxLen--;


        /*
         * All bytes have entered the USART.
         *
         * Now wait for TC so we know the final bit
         * has actually left the UART.
         */

        if(pUSARTHandle->TxLen == 0)
        {
            /*
             * Disable TXE interrupt.
             */

            pUSARTHandle->pUSARTx->CR1 &=
                ~(1 << USART_IRQ_TXE);


            /*
             * Enable TC interrupt.
             */

            pUSARTHandle->pUSARTx->CR1 |=
                (1 << USART_IRQ_TC);
        }
    }


    /*
     * ========================================================
     * TC INTERRUPT
     * ========================================================
     */

    if(USART_GetFlagStatus(
            pUSARTHandle->pUSARTx,
            USART_FLAG_TC)
       &&
       (pUSARTHandle->pUSARTx->CR1 &
        (1 << USART_IRQ_TC)))
    {
        /*
         * TX is completely finished.
         */

        pUSARTHandle->TxBusyState =
            USART_READY;


        /*
         * Disable TC interrupt.
         */

        pUSARTHandle->pUSARTx->CR1 &=
            ~(1 << USART_IRQ_TC);


        /*
         * Notify application.
         */

        USART_ApplicationEventCallback(
            pUSARTHandle,
            USART_EVENT_TX_CMPLT,
            0
        );
    }


    /*
     * ========================================================
     * RXNE INTERRUPT
     * ========================================================
     */

    rxne_status =
        USART_GetFlagStatus(
            pUSARTHandle->pUSARTx,
            USART_FLAG_RXNE
        );

    rxneie_status =
        (pUSARTHandle->pUSARTx->CR1 &
        (1 << USART_IRQ_RXNE))
        >> USART_IRQ_RXNE;


    if(rxne_status && rxneie_status)
    {
        uint8_t receivedByte;


        /*
         * Read the received byte ONCE from DR.
         */

        receivedByte =
            (uint8_t)(pUSARTHandle->pUSARTx->DR & 0xFF);


        /*
         * --------------------------------------------------------
         * NEW:
         * Pass every received byte to the application.
         *
         * JRD100_ProcessByte() will receive this byte through
         * USART_ApplicationEventCallback().
         * --------------------------------------------------------
         */

        USART_ApplicationEventCallback(
            pUSARTHandle,
            USART_EVENT_RX_BYTE,
            receivedByte
        );


        /*
         * --------------------------------------------------------
         * OLD FIXED-LENGTH RX LOGIC
         *
         * Keep this because our loopback test already proved
         * that this part works.
         * --------------------------------------------------------
         */

        if(pUSARTHandle->RxBusyState ==
           USART_BUSY_IN_RX)
        {
            *(pUSARTHandle->pRxBuffer) =
                receivedByte;

            pUSARTHandle->pRxBuffer++;

            pUSARTHandle->RxLen--;


            if(pUSARTHandle->RxLen == 0)
            {
                pUSARTHandle->RxBusyState =
                    USART_READY;

                pUSARTHandle->pUSARTx->CR1 &=
                    ~(1 << USART_IRQ_RXNE);


                USART_ApplicationEventCallback(
                    pUSARTHandle,
                    USART_EVENT_RX_CMPLT,
                    0
                );
            }
        }
    }

}



void USART_SetBaudRate(USART_RegDef_t *pUSARTx,uint32_t BaudRate)
{
    uint32_t PCLKx;
    uint32_t usartdiv;
    uint32_t mantissa;
    uint32_t fraction;

    /*
     * Get peripheral clock.
     *
     * USART1 and USART6 are on APB2.
     * All other USART/UART peripherals are on APB1.
     */
    if((pUSARTx == USART1) || (pUSARTx == USART6))
    {
        PCLKx = RCC_GetPCLK2Value();
    }
    else
    {
        PCLKx = RCC_GetPCLK1Value();
    }


    /*
     * Oversampling by 16
     */
    if(!(pUSARTx->CR1 & (1 << USART_CR1_OVER8)))
    {
        usartdiv = ((PCLKx * 25) / (4 * BaudRate));

        mantissa = usartdiv / 100;

        fraction = usartdiv - (mantissa * 100);

        fraction = (((fraction * 16) + 50) / 100) & 0xF;

        pUSARTx->BRR = ((mantissa << 4) | fraction);
    }

    /*
     * Oversampling by 8
     */
    else
    {
        usartdiv = ((PCLKx * 25) / (2 * BaudRate));

        mantissa = usartdiv / 100;

        fraction = usartdiv - (mantissa * 100);

        fraction = (((fraction * 8) + 50) / 100) & 0x7;

        pUSARTx->BRR = ((mantissa << 4) | ((fraction & 0x7) << 1));
    }
}
