/*
 * stm32f407xx_spi_driver.c
 *
 *  Created on: Oct 18, 2025
 *      Author: anas
 */
#include "stm32f429xx_driver_spi.h"
#include "stm32f429xx.h"

static void spi_txe_interrupt_handle(SPI_Handle_t *pSPIHandle);
static void spi_rxne_interrupt_handle(SPI_Handle_t *pSPIHandle);
static void spi_ovr_err_interrupt_handle(SPI_Handle_t *pSPIHandle);

void SPI_PeriClockControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
		{
			if(pSPIx==SPI1)
			{
			SPI1_PCLK_EN();
			}
			else if(pSPIx==SPI2)
			{
			SPI2_PCLK_EN();
			}
			else if(pSPIx==SPI3)
			{
			SPI3_PCLK_EN();
			}

		}
		else
		{
			if(pSPIx==SPI1)
			{
				SPI1_PCLK_DI();
			}
			else if(pSPIx==SPI2)
			{
				SPI2_PCLK_DI();
			}
			else if(pSPIx==SPI3)
			{
				SPI3_PCLK_DI();
			}

		}
}

void SPI_Init(SPI_Handle_t *pSPIHandle)
{
	SPI_PeriClockControl(pSPIHandle->pSPIx, ENABLE);

	uint32_t tempreg = 0;
	tempreg |= pSPIHandle->SPIConfig.SPI_DeviceMode << SPI_CR1_MSTR;

	if(pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_FD)
	{
		tempreg &= ~(1 << SPI_CR1_BIDIMODE);
	}else if(pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_HD)
	{
		tempreg |= 1 << SPI_CR1_BIDIMODE;
	}else if(pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_SIMPLEX_RXONLY)
	{
		tempreg &= ~(1 << SPI_CR1_BIDIMODE);
		tempreg |= 1 << SPI_CR1_RXONLY;
	}

	tempreg |= pSPIHandle->SPIConfig.SPI_SclkSpeed << SPI_CR1_BR;

	tempreg |= pSPIHandle->SPIConfig.SPI_DFF << SPI_CR1_DFF;

	tempreg |= pSPIHandle->SPIConfig.SPI_CPOL << SPI_CR1_CPOL;

	tempreg |= pSPIHandle->SPIConfig.SPI_CPHA << SPI_CR1_CPHA;

	tempreg |= pSPIHandle->SPIConfig.SPI_SSM << SPI_CR1_SSM;

	pSPIHandle->pSPIx->CR1 = tempreg;


}

void SPI_DeInit(SPI_RegDef_t *pSPIx)
{
	if(pSPIx == SPI1)
	{
	SPI1_REG_RESET();
	}else if(pSPIx == SPI2)
	{
	SPI2_REG_RESET();
	}else if(pSPIx == SPI3)
	{
	SPI3_REG_RESET();
	}
}

uint8_t SPI_GetFlagStatus(SPI_RegDef_t *pSPIx, uint32_t FlagName)
{
	if(pSPIx->SR & FlagName)
	{
		return FLAG_SET;
	}
	return FLAG_RESET;
}

void SPI_SendData(SPI_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t Len)
{
	while(Len > 0)
	{
		while(SPI_GetFlagStatus(pSPIx,SPI_TXE_FLAG) == FLAG_RESET);

		if(pSPIx->CR1 & (1 << SPI_CR1_DFF))
		{
			pSPIx->DR = *((uint16_t*)pTxBuffer);
			Len--;
			Len--;
			(uint16_t*)pTxBuffer++;
		}else
		{
			pSPIx->DR = *pTxBuffer;
			Len--;
			pTxBuffer++;
		}




	}
}

void SPI_ReceiveData(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t Len)
{
	while(Len > 0)
	{
		while(SPI_GetFlagStatus(pSPIx,SPI_RXNE_FLAG) == FLAG_RESET);

		if(pSPIx->CR1 & (1 << SPI_CR1_DFF))
		{
			*((uint16_t*)pRxBuffer) = pSPIx->DR;
			Len--;
			Len--;
			(uint16_t*)pRxBuffer++;
		}else
		{
			*pRxBuffer = pSPIx->DR;
			Len--;
			pRxBuffer++;
		}




	}
}

uint8_t SPI_SendDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pTxBuffer, uint32_t Len)
{
	uint8_t state = pSPIHandle->TxState;

	if(state != SPI_BUSY_IN_TX)
	{
		//1 . Save the Tx buffer address and Len information in some global variables
		pSPIHandle->pTxBuffer = pTxBuffer;
		pSPIHandle->TxLen = Len;
		//2.  Mark the SPI state as busy in transmission so that
		//    no other code can take over same SPI peripheral until transmission is over
		pSPIHandle->TxState = SPI_BUSY_IN_TX;

		//3. Enable the TXEIE control bit to get interrupt whenever TXE flag is set in SR
		pSPIHandle->pSPIx->CR2 |= ( 1 << SPI_CR2_TXEIE );
	}
	return state;
}

uint8_t SPI_ReceiveDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pRxBuffer, uint32_t Len)
{
	uint8_t state = pSPIHandle->RxState;

	if(state != SPI_BUSY_IN_RX)
	{
		//1 . Save the Tx buffer address and Len information in some global variables
		pSPIHandle->pRxBuffer = pRxBuffer;
		pSPIHandle->RxLen = Len;
		//2.  Mark the SPI state as busy in transmission so that
		//    no other code can take over same SPI peripheral until transmission is over
		pSPIHandle->RxState = SPI_BUSY_IN_RX;

		//3. Enable the TXEIE control bit to get interrupt whenever TXE flag is set in SR
		pSPIHandle->pSPIx->CR2 |= ( 1 << SPI_CR2_RXNEIE );
	}
	return state;
}

void SPI_IRQConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
		{
			if(IRQNumber <= 31)
			{
				*NVIC_ISER0 |= (1 << IRQNumber);

			}else if(IRQNumber > 31 && 64 > IRQNumber)
			{
				*NVIC_ISER1 |= (1 << (IRQNumber%32));

			}else if(IRQNumber >= 64 && 96 > IRQNumber)
			{
				*NVIC_ISER2 |= (1 << (IRQNumber%64));
			}

		}else
		{
			if(IRQNumber <= 31)
			{
				*NVIC_ICER0 |= (1 << IRQNumber);

			}else if(IRQNumber > 31 && 64 > IRQNumber)
			{
				*NVIC_ICER1 |= (1 << (IRQNumber%32));

			}else if(IRQNumber >= 64 && 96 > IRQNumber)
			{
				*NVIC_ICER2 |= (1 << (IRQNumber%64));

			}
		}
}

void SPI_IRQPriorityConfig(uint8_t IRQNumber,uint32_t IRQPriority)
{
	uint8_t iprx = IRQNumber/4;
	uint8_t iprx_section = IRQNumber%4;
	uint8_t shift_amount = (iprx_section * 8) + (8 - NO_PR_BITS_IMPLEMENTED);
	*(NVIC_PR_BASE_ADDR + (iprx)) |= IRQPriority << shift_amount;
}

void SPI_IRQHandling(SPI_Handle_t *pHandle)
{
	uint8_t temp1 , temp2;
	//first lets check txe
	temp1 = pHandle->pSPIx->SR & ( 1 << SPI_SR_TXE);
	temp2 = pHandle->pSPIx->CR2 & ( 1 << SPI_CR2_TXEIE);

	if(temp1 && temp2)
	{
		//handle txe
		spi_txe_interrupt_handle(pHandle);
	}

	//check for rxne
	temp1 = pHandle->pSPIx->SR & ( 1 << SPI_SR_RXNE);
	temp2 = pHandle->pSPIx->CR2 & ( 1 << SPI_CR2_RXNEIE);

	if(temp1 && temp2)
	{
		//handle rxne
		spi_rxne_interrupt_handle(pHandle);
	}

	//check for ovr flag
	temp1 = pHandle->pSPIx->SR & ( 1 << SPI_SR_OVR);
	temp2 = pHandle->pSPIx->CR2 & ( 1 << SPI_CR2_ERRIE);

	if(temp1 && temp2)
	{
		//handle ovr error
		spi_ovr_err_interrupt_handle(pHandle);
	}

}

//some helper function implementation

static void spi_txe_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
	if(pSPIHandle->pSPIx->CR1 & (1 << SPI_CR1_DFF))
	{
		pSPIHandle->pSPIx->DR = *((uint16_t*)pSPIHandle->pTxBuffer);
		pSPIHandle->TxLen--;
		pSPIHandle->TxLen--;
		(uint16_t*)pSPIHandle->pTxBuffer++;
	}else
	{
		pSPIHandle->pSPIx->DR = *pSPIHandle->pTxBuffer;
		pSPIHandle->TxLen--;
		pSPIHandle->pTxBuffer++;
	}
	if(! pSPIHandle->TxLen)
	{
		//txlen is zero , so closebthe spi transmission and inform the applixation that
		//TX is over

		//this prevents interrupts from setting up of txe flag
		pSPIHandle->pSPIx->CR2 &= ~(1 << SPI_CR2_TXEIE);
		pSPIHandle->pTxBuffer = NULL;
		pSPIHandle->TxLen = 0;
		pSPIHandle->TxState = SPI_READY;
		SPI_CloseTransmission(pSPIHandle);
		SPI_ApplicationEventCallback(pSPIHandle,SPI_EVENT_TX_CMPLT);
	}
}
static void spi_rxne_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
	if(pSPIHandle->pSPIx->CR1 & ( 1 << 11))
	{
		//16 bit
		*((uint16_t*)pSPIHandle->pRxBuffer) = (uint16_t) pSPIHandle->pSPIx->DR;
		pSPIHandle->RxLen -= 2;
		pSPIHandle->pRxBuffer++;
		pSPIHandle->pRxBuffer++;

	}else
	{
		//8 bit
		*(pSPIHandle->pRxBuffer) = (uint8_t) pSPIHandle->pSPIx->DR;
		pSPIHandle->RxLen--;
		pSPIHandle->pRxBuffer++;
	}

	if(! pSPIHandle->RxLen)
	{
		//reception is complete
		SPI_CloseReception(pSPIHandle);
		SPI_ApplicationEventCallback(pSPIHandle,SPI_EVENT_RX_CMPLT);
	}
}
static void spi_ovr_err_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
	uint8_t temp;
	//1. clear the ovr flag
	if(pSPIHandle->TxState != SPI_BUSY_IN_TX)
	{
		temp = pSPIHandle->pSPIx->DR;
		temp = pSPIHandle->pSPIx->SR;
	}
	(void)temp;
	//2. inform the application
	SPI_ApplicationEventCallback(pSPIHandle,SPI_EVENT_OVR_ERR);
}

void SPI_CloseTransmission(SPI_Handle_t *pSPIHandle)
{
	pSPIHandle->pSPIx->CR2 &= ~(1 << SPI_CR2_TXEIE);
	pSPIHandle->pTxBuffer = NULL;
	pSPIHandle->TxLen = 0;
	pSPIHandle->TxState = SPI_READY;
}
void SPI_CloseReception(SPI_Handle_t *pSPIHandle)
{
	pSPIHandle->pSPIx->CR2 &= ~(1 << SPI_CR2_RXNEIE);
	pSPIHandle->pRxBuffer = NULL;
	pSPIHandle->RxLen = 0;
	pSPIHandle->RxState = SPI_READY;
}

void SPI_ClearOVRFlag(SPI_RegDef_t *pSPIx)
{
	uint8_t temp;
	temp = pSPIx->DR;
	temp = pSPIx->SR;
	(void)temp;
}

void SPI_PeripheralControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		pSPIx->CR1 |= 1 << SPI_CR1_SPE;
	}else
	{
		pSPIx->CR1 &= ~(1 << SPI_CR1_SPE);
	}
}

void SPI_SSIConfig(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		pSPIx->CR1 |= 1 << SPI_CR1_SSI;
	}else
	{
		pSPIx->CR1 &= ~(1 << SPI_CR1_SSI);
	}
}

void SPI_SSOEConfig(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		pSPIx->CR2 |= 1 << SPI_CR2_SSOE;
	}else
	{
		pSPIx->CR2 &= ~(1 << SPI_CR2_SSOE);
	}
}

__attribute__((weak)) void SPI_ApplicationEventCallback(SPI_Handle_t *pSPIHandle,uint8_t AppEv)
{

}
